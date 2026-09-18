#pragma once

#include "Compressor.h"
#include "portaudio.h"
#include <atomic>
#include <string>

// Control methods are used on the UI thread. Only atomics cross into the callback.
class AudioEngine {
public:
    AudioEngine() = default;
    ~AudioEngine();
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool startAudio();
    void stopAudio();
    bool startRecording();
    bool stopRecording();
    bool isRunning() const;
    bool isRecording() const { return recording_; }
    std::string status() const;
    void setParameters(float threshold, float ratio, float knee, float makeup);
    float inputPeak() const { return inputPeak_.load(std::memory_order_relaxed); }
    float outputPeak() const { return outputPeak_.load(std::memory_order_relaxed); }

private:
    static int callback(const void*, void*, unsigned long,
                        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*) noexcept;
    bool pauseStream();
    bool resumeStream();
    bool finishRecording();
    void setError(const char* operation, PaError error);
    PaStream* stream_ = nullptr;
    bool initialized_ = false;
    bool recording_ = false;
    Compressor compressor_{false};
    WavRecorder original_, compressed_;
    std::atomic<float> threshold_{-20}, ratio_{5}, knee_{0}, makeup_{3};
    std::atomic<float> inputPeak_{0}, outputPeak_{0};
    std::atomic<bool> callbackFailed_{false};
    std::string status_ = "Ready. Start audio to monitor the default stereo devices.";
    std::string recordingPath_;
};
