#include "AudioEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>

namespace {
constexpr unsigned channels = 2;
constexpr double sampleRate = 44100;
}

AudioEngine::~AudioEngine() {
    stopAudio();
    if (initialized_) Pa_Terminate();
}

void AudioEngine::setError(const char* operation, PaError error) {
    status_ = std::string(operation) + ": " + Pa_GetErrorText(error);
}

std::string AudioEngine::status() const {
    if (callbackFailed_.load(std::memory_order_relaxed))
        return "Audio processing stopped unexpectedly. Stop audio before restarting.";
    return status_;
}

bool AudioEngine::isRunning() const {
    return stream_ && Pa_IsStreamActive(stream_) == 1;
}

void AudioEngine::setParameters(float threshold, float ratio, float knee, float makeup) {
    if (std::isfinite(threshold)) threshold_.store(std::clamp(threshold, -80.0f, 0.0f), std::memory_order_relaxed);
    if (std::isfinite(ratio)) ratio_.store(std::clamp(ratio, 1.0f, 20.0f), std::memory_order_relaxed);
    if (std::isfinite(knee)) knee_.store(std::clamp(knee, 0.0f, 40.0f), std::memory_order_relaxed);
    if (std::isfinite(makeup)) makeup_.store(std::clamp(makeup, -24.0f, 24.0f), std::memory_order_relaxed);
}

bool AudioEngine::startAudio() {
    if (isRunning()) return true;
    if (stream_) stopAudio();
    callbackFailed_.store(false, std::memory_order_relaxed);
    if (!initialized_) {
        const auto error = Pa_Initialize();
        if (error != paNoError) { setError("Audio initialization failed", error); return false; }
        initialized_ = true;
    }
    const auto inputDevice = Pa_GetDefaultInputDevice();
    const auto outputDevice = Pa_GetDefaultOutputDevice();
    const auto* inputInfo = Pa_GetDeviceInfo(inputDevice);
    const auto* outputInfo = Pa_GetDeviceInfo(outputDevice);
    if (!inputInfo || !outputInfo || inputInfo->maxInputChannels < 2 || outputInfo->maxOutputChannels < 2) {
        status_ = "Default input and output must both support stereo. Select stereo devices in system settings.";
        return false;
    }
    PaStreamParameters input{inputDevice, channels, paFloat32, inputInfo->defaultLowInputLatency, nullptr};
    PaStreamParameters output{outputDevice, channels, paFloat32, outputInfo->defaultLowOutputLatency, nullptr};
    auto error = Pa_IsFormatSupported(&input, &output, sampleRate);
    if (error != paFormatIsSupported) { setError("Stereo 44.1 kHz format unavailable", error); return false; }
    error = Pa_OpenStream(&stream_, &input, &output, sampleRate, 256, paClipOff, callback, this);
    if (error != paNoError) { stream_ = nullptr; setError("Opening audio failed", error); return false; }
    if (!resumeStream()) return false;
    status_ = "Monitoring: " + std::string(inputInfo->name) + " → " + outputInfo->name;
    return true;
}

bool AudioEngine::pauseStream() {
    if (!stream_) return false;
    auto error = Pa_StopStream(stream_);
    if (error == paNoError || error == paStreamIsStopped) return true;
    // Closing an errored stream also guarantees callbacks have finished.
    Pa_AbortStream(stream_);
    Pa_CloseStream(stream_);
    stream_ = nullptr;
    setError("Stopping audio failed", error);
    return false;
}

bool AudioEngine::resumeStream() {
    if (!stream_) return false;
    const auto error = Pa_StartStream(stream_);
    if (error == paNoError) return true;
    Pa_CloseStream(stream_);
    stream_ = nullptr;
    setError("Starting audio failed", error);
    return false;
}

bool AudioEngine::finishRecording() {
    if (!recording_) return true;
    recording_ = false;
    const bool originalSaved = original_.stopRecording();
    const bool compressedSaved = compressed_.stopRecording();
    status_ = originalSaved && compressedSaved ? "Saved recordings to " + recordingPath_
                                               : "Recording could not be saved completely: " + recordingPath_;
    return originalSaved && compressedSaved;
}

void AudioEngine::stopAudio() {
    bool stopped = true;
    if (stream_) {
        stopped = pauseStream();
        if (stream_) { Pa_CloseStream(stream_); stream_ = nullptr; }
    }
    const auto stopError = status_;
    const bool hadRecording = recording_;
    finishRecording();
    if (!stopped) status_ = stopError;
    inputPeak_.store(0, std::memory_order_relaxed);
    outputPeak_.store(0, std::memory_order_relaxed);
    callbackFailed_.store(false, std::memory_order_relaxed);
    if (!hadRecording && stopped) status_ = "Audio stopped.";
}

bool AudioEngine::startRecording() {
    if (recording_) return true;
    if (!isRunning()) { status_ = "Start audio before recording."; return false; }
    if (!pauseStream()) return false;
    bool opened = false;
    try {
        const auto root = std::filesystem::current_path() / "recordings";
        std::filesystem::create_directories(root);
        const auto stamp = std::chrono::system_clock::now().time_since_epoch().count();
        // Atomically reserve a directory, including when two app instances record.
        for (unsigned attempt = 0; attempt < 1000; ++attempt) {
            const auto directory = root / ("session-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            if (!std::filesystem::create_directory(directory)) continue;
            recordingPath_ = directory.string();
            opened = original_.startRecording((directory / "original.wav").string().c_str()) &&
                     compressed_.startRecording((directory / "compressed.wav").string().c_str());
            break;
        }
        if (!opened) { original_.stopRecording(); compressed_.stopRecording(); }
    } catch (const std::exception& error) {
        original_.stopRecording();
        compressed_.stopRecording();
        status_ = std::string("Cannot create recording: ") + error.what();
        resumeStream();
        return false;
    }
    recording_ = opened;
    if (!resumeStream()) {
        const auto error = status_;
        finishRecording();
        status_ = error;
        return false;
    }
    status_ = opened ? "Recording to " + recordingPath_ : "Cannot create recording files.";
    return opened;
}

bool AudioEngine::stopRecording() {
    if (!recording_) return true;
    const bool paused = pauseStream();
    const auto pauseError = status_;
    const bool saved = finishRecording();
    if (!paused) status_ = pauseError;
    const bool resumed = paused && resumeStream();
    return saved && resumed;
}

int AudioEngine::callback(const void* inputBuffer, void* outputBuffer, unsigned long frames,
                          const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* context) noexcept {
    auto& self = *static_cast<AudioEngine*>(context);
    auto* output = static_cast<float*>(outputBuffer);
    const auto* input = static_cast<const float*>(inputBuffer);
    if (!output) return paAbort;
    try {
        self.compressor_.threshold = self.threshold_.load(std::memory_order_relaxed);
        self.compressor_.ratio = self.ratio_.load(std::memory_order_relaxed);
        self.compressor_.knee = self.knee_.load(std::memory_order_relaxed);
        self.compressor_.makeUpGain = self.makeup_.load(std::memory_order_relaxed);
        if (!input) std::fill_n(output, frames * channels, 0.0f);
        if (self.recording_) self.original_.AppendData(input ? input : output, frames);
        self.compressor_.process(input, output, static_cast<unsigned>(frames), channels);
        if (self.recording_) self.compressed_.AppendData(output, frames);
        float inputPeak = 0, outputPeak = 0;
        for (unsigned long i = 0; i < frames * channels; ++i) {
            if (input) inputPeak = std::max(inputPeak, std::fabs(input[i]));
            outputPeak = std::max(outputPeak, std::fabs(output[i]));
        }
        const float decay = std::exp(-static_cast<float>(frames) / (0.3f * sampleRate));
        self.inputPeak_.store(std::max(inputPeak, self.inputPeak() * decay), std::memory_order_relaxed);
        self.outputPeak_.store(std::max(outputPeak, self.outputPeak() * decay), std::memory_order_relaxed);
        return paContinue;
    } catch (...) {
        std::fill_n(output, frames * channels, 0.0f);
        self.callbackFailed_.store(true, std::memory_order_relaxed);
        return paAbort;
    }
}
