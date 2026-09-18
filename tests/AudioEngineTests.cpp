#include "AudioEngine.h"
#include "portaudio.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}
struct TemporaryDirectory {
    fs::path previous = fs::current_path();
    fs::path path;
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            path = fs::temp_directory_path() / ("audio-engine-tests-" +
                std::to_string(stamp) + "-" + std::to_string(attempt));
            if (fs::create_directory(path)) {
                fs::current_path(path);
                return;
            }
        }
        throw std::runtime_error("could not create temporary directory");
    }
    ~TemporaryDirectory()
    {
        std::error_code ignored;
        fs::current_path(previous, ignored);
        fs::remove_all(path, ignored);
    }
};
struct FakeAudio {
    PaError initializeError = paNoError;
    PaError openError = paNoError;
    PaError startError = paNoError;
    PaError stopError = paNoError;
    PaError formatError = paNoError;
    bool noInput = false;
    bool active = false;
    int initializeCalls = 0;
    int terminateCalls = 0;
    int openCalls = 0;
    int closeCalls = 0;
    int stopCalls = 0;
    int abortCalls = 0;
    int inputChannels = 0;
    int outputChannels = 0;
    PaStreamCallback* callback = nullptr;
    void* userData = nullptr;
    PaDeviceInfo device{1, "Test stereo device", 0, 2, 2, 0.01, 0.01, 0.1, 0.1, 44100.0};
    std::vector<float> process(const std::vector<float>& input)
    {
        require(active && callback, "callback requires active stream");
        std::vector<float> output(input.size() / inputChannels * outputChannels);
        PaStreamCallbackTimeInfo time{};
        require(callback(input.data(), output.data(), input.size() / inputChannels,
                         &time, 0, userData) == paContinue, "callback continues");
        return output;
    }
} fake;
}

extern "C" {
PaError Pa_Initialize() { ++fake.initializeCalls; return fake.initializeError; }
PaError Pa_Terminate() { ++fake.terminateCalls; return paNoError; }
const char* Pa_GetErrorText(PaError) { return "simulated audio error"; }
PaDeviceIndex Pa_GetDefaultInputDevice() { return fake.noInput ? paNoDevice : 0; }
PaDeviceIndex Pa_GetDefaultOutputDevice() { return 0; }
PaDeviceIndex Pa_GetDeviceCount() { return 1; }
const PaDeviceInfo* Pa_GetDeviceInfo(PaDeviceIndex index)
{ return index == 0 ? &fake.device : nullptr; }
PaError Pa_IsFormatSupported(const PaStreamParameters*, const PaStreamParameters*, double)
{ return fake.formatError; }
PaError Pa_OpenStream(PaStream** stream, const PaStreamParameters* input,
                      const PaStreamParameters* output, double, unsigned long,
                      PaStreamFlags, PaStreamCallback* callback, void* userData)
{
    ++fake.openCalls;
    if (fake.openError != paNoError) return fake.openError;
    fake.inputChannels = input->channelCount;
    fake.outputChannels = output->channelCount;
    fake.callback = callback;
    fake.userData = userData;
    *stream = reinterpret_cast<PaStream*>(&fake);
    return paNoError;
}
PaError Pa_StartStream(PaStream*)
{
    if (fake.startError != paNoError) return fake.startError;
    fake.active = true;
    return paNoError;
}
PaError Pa_StopStream(PaStream*)
{
    ++fake.stopCalls;
    if (fake.stopError != paNoError) return fake.stopError;
    fake.active = false;
    return paNoError;
}
PaError Pa_AbortStream(PaStream*)
{ ++fake.abortCalls; fake.active = false; return paNoError; }
PaError Pa_CloseStream(PaStream*)
{ ++fake.closeCalls; fake.active = false; return paNoError; }
PaError Pa_IsStreamActive(PaStream*) { return fake.active ? 1 : 0; }
PaError Pa_IsStreamStopped(PaStream*) { return fake.active ? 0 : 1; }
}

namespace {
std::vector<fs::path> recordings()
{
    std::vector<fs::path> result;
    if (fs::exists("recordings"))
        for (const auto& entry : fs::recursive_directory_iterator("recordings"))
            if (entry.is_regular_file()) result.push_back(entry.path());
    return result;
}
void testFailures()
{
    fake = {};
    {
        AudioEngine engine;
        require(fake.initializeCalls == 0 && fs::is_empty(fs::current_path()),
                "construction must not initialize audio or create files");
        require(!engine.startRecording(), "recording needs running audio");
        fake.initializeError = paInternalError;
        require(!engine.startAudio() && !engine.isRunning(), "initialization failure");
        require(!engine.status().empty(), "initialization error is visible");
    }
    require(fake.terminateCalls == 0, "failed initialization must not terminate PortAudio");
    fake = {};
    {
        AudioEngine engine;
        fake.noInput = true;
        require(!engine.startAudio(), "missing input rejected");
        fake.noInput = false;
        fake.device.maxInputChannels = 1;
        require(!engine.startAudio(), "mono input rejected");
        fake.device.maxInputChannels = 2;
        fake.formatError = paInvalidSampleRate;
        require(!engine.startAudio(), "unsupported format rejected");
        require(fake.openCalls == 0, "invalid configuration never opens stream");
        fake.formatError = paNoError;
        fake.openError = paInternalError;
        require(!engine.startAudio() && !engine.isRunning(), "open failure");
        fake.openError = paNoError;
        fake.startError = paInternalError;
        require(!engine.startAudio() && fake.closeCalls == 1, "failed start closes stream");
        fake.startError = paNoError;
        require(engine.startAudio(), "recover from startup failures");
    }
    require(!fake.active && fake.closeCalls == 2 && fake.terminateCalls == 1,
            "destructor closes recovered stream and balances initialization");
    require(fs::is_empty(fs::current_path()), "monitoring and failures create no recordings");
}
void testProcessingAndLifecycle()
{
    fake = {};
    AudioEngine engine;
    require(engine.startAudio() && engine.isRunning(), "start monitoring");
    require(engine.startAudio() && fake.openCalls == 1, "repeated start is idempotent");
    engine.setParameters(-20, 4, 0, 0);
    const auto output = fake.process({1.0f, -1.0f, 0.01f, -0.01f});
    const float compressed = std::pow(10.0f, -15.0f / 20.0f);
    require(std::fabs(output[0] - compressed) < 1e-5f &&
            std::fabs(output[1] + compressed) < 1e-5f,
            "UI parameters affect compression and preserve polarity");
    require(std::fabs(output[2] - 0.01f) < 1e-6f, "below-threshold signal passes");
    require(engine.inputPeak() == 1.0f && std::fabs(engine.outputPeak() - compressed) < 1e-5f,
            "meters reflect callback input and output");
    engine.setParameters(-20, 1, 0, 0);
    require(std::fabs(fake.process({0.5f, -0.5f})[0] - 0.5f) < 1e-6f,
            "parameter changes take effect on next callback");
    float silence[4] = {1, 1, 1, 1};
    PaStreamCallbackTimeInfo time{};
    require(fake.callback(nullptr, silence, 2, &time, 0, fake.userData) == paContinue,
            "missing input handled");
    for (float sample : silence) require(sample == 0, "missing input emits silence");
    engine.stopAudio();
    engine.stopAudio();
    require(!engine.isRunning() && fake.closeCalls == 1, "repeated stop closes once");
    require(engine.inputPeak() == 0 && engine.outputPeak() == 0, "stopped meters reset");
    require(engine.startAudio(), "restart after stopping");
    fake.stopError = paInternalError;
    engine.stopAudio();
    require(!fake.active && fake.abortCalls == 1 && fake.closeCalls == 2,
            "stop failure aborts and closes stream");
}
void testRecordingSessions()
{
    fake = {};
    AudioEngine engine;
    require(engine.startAudio(), "start recording test audio");
    require(engine.startRecording() && engine.isRecording(), "start first recording");
    require(fake.stopCalls == 1 && fake.active, "recording setup pauses and resumes callbacks");
    fake.process({0.2f, -0.2f, 0.4f, -0.4f});
    require(engine.stopRecording() && !engine.isRecording() && engine.isRunning(),
            "save recording and keep monitoring");
    require(fake.stopCalls == 2, "saving pauses callbacks");
    const auto first = recordings();
    require(first.size() == 2, "first session has input and output WAVs");
    std::vector<std::uintmax_t> sizes;
    for (const auto& path : first) {
        sizes.push_back(fs::file_size(path));
        require(sizes.back() > 44 + 16, "recorded WAV includes header and samples");
    }
    require(engine.startRecording(), "start second recording");
    fake.process({0.1f, -0.1f});
    engine.stopAudio();
    require(!engine.isRecording() && !engine.isRunning(), "stop audio finalizes recording");
    require(recordings().size() == 4, "sessions use distinct paths");
    for (std::size_t i = 0; i < first.size(); ++i)
        require(fs::file_size(first[i]) == sizes[i], "later recording preserves first session");
    require(engine.stopRecording(), "stop recording is idempotent");
    require(engine.startAudio(), "restart audio for recording failure");
    fake.startError = paInternalError;
    require(!engine.startRecording() && !engine.isRecording() && !engine.isRunning(),
            "recording resume failure closes stream and finalizes files");
    require(engine.status().find("Starting audio failed") != std::string::npos,
            "recording finalization preserves resume error");
    fake.startError = paNoError;
    require(engine.startAudio() && engine.startRecording(), "recover recording after resume failure");
    fake.process({0.1f, -0.1f});
    fake.stopError = paInternalError;
    require(!engine.stopRecording() && !engine.isRecording() && !engine.isRunning(),
            "recording pause failure aborts audio and finalizes files");
    require(engine.status().find("Stopping audio failed") != std::string::npos,
            "recording finalization preserves stop error");
}
}
int main()
{
    try {
        TemporaryDirectory temporary;
        testFailures();
        testProcessingAndLifecycle();
        testRecordingSessions();
    } catch (const std::exception& error) {
        std::cerr << "AudioEngine regression failure: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
