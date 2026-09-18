#include "WavRecorder.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct TemporaryDirectory {
    fs::path path;
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            path = fs::temp_directory_path() / ("wav-recorder-tests-" +
                std::to_string(stamp) + "-" + std::to_string(attempt));
            if (fs::create_directory(path))
                return;
        }
        throw std::runtime_error("could not create temporary directory");
    }
    ~TemporaryDirectory()
    {
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }
};

std::uint32_t littleEndian(const std::vector<unsigned char>& bytes, std::size_t offset,
                         std::size_t size)
{
    require(offset <= bytes.size() && size <= bytes.size() - offset, "truncated WAV field");
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < size; ++i)
        value |= static_cast<std::uint32_t>(bytes[offset + i]) << (8 * i);
    return value;
}

void verifyWav(const fs::path& path, std::uint32_t sampleRate, std::uint16_t channels,
               const std::vector<float>& samples)
{
    std::ifstream input(path, std::ios::binary);
    require(input.is_open(), "missing WAV file: " + path.string());
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), {});
    require(bytes.size() >= 12, "WAV header missing");
    require(std::string(bytes.begin(), bytes.begin() + 4) == "RIFF", "RIFF signature");
    require(std::string(bytes.begin() + 8, bytes.begin() + 12) == "WAVE", "WAVE signature");
    require(littleEndian(bytes, 4, 4) == bytes.size() - 8, "RIFF size must match file");

    bool foundFormat = false, foundFact = false, foundData = false;
    std::size_t offset = 12;
    while (offset < bytes.size()) {
        require(bytes.size() - offset >= 8, "truncated chunk header");
        const std::string id(bytes.begin() + offset, bytes.begin() + offset + 4);
        const auto size = littleEndian(bytes, offset + 4, 4);
        offset += 8;
        require(size <= bytes.size() - offset, "chunk extends past end of file");
        if (id == "fmt ") {
            require(!foundFormat && size >= 18, "float WAV requires one extended fmt chunk");
            foundFormat = true;
            require(littleEndian(bytes, offset, 2) == 3, "IEEE float format");
            require(littleEndian(bytes, offset + 2, 2) == channels, "channel count");
            require(littleEndian(bytes, offset + 4, 4) == sampleRate, "sample rate");
            require(littleEndian(bytes, offset + 8, 4) == sampleRate * channels * 4, "byte rate");
            require(littleEndian(bytes, offset + 12, 2) == channels * 4u, "block alignment");
            require(littleEndian(bytes, offset + 14, 2) == 32, "sample bit depth");
            require(littleEndian(bytes, offset + 16, 2) == 0, "format extension size");
        } else if (id == "fact") {
            require(!foundFact && size >= 4, "one complete fact chunk");
            foundFact = true;
            require(littleEndian(bytes, offset, 4) == samples.size() / channels, "fact frame count");
        } else if (id == "data") {
            require(!foundData, "one data chunk");
            foundData = true;
            require(size == samples.size() * 4, "audio data size");
            for (std::size_t i = 0; i < samples.size(); ++i) {
                std::uint32_t expectedBits;
                static_assert(sizeof(float) == sizeof(expectedBits), "32-bit float required");
                std::memcpy(&expectedBits, &samples[i], sizeof(expectedBits));
                require(littleEndian(bytes, offset + i * 4, 4) == expectedBits,
                        "sample data and channel order at index " + std::to_string(i));
            }
        }
        offset += size + (size & 1u);
    }
    require(offset == bytes.size(), "chunk padding must fit file");
    require(foundFormat && foundFact && foundData, "required WAV chunks");
}
} // namespace

int main()
{
    try {
        TemporaryDirectory temporary;
        WavRecorder recorder;
        const float stereo[] = {0.0f, -0.0f, 0.25f, -0.5f, 1.0f, -1.0f};
        recorder.AppendData(stereo, 3);
        require(recorder.stopRecording(), "stop before start");

        const auto first = temporary.path / "stereo.wav";
        require(recorder.startRecording(first.string().c_str()), "start stereo recording");
        recorder.AppendData(stereo, 1);
        recorder.AppendData(nullptr, 2);
        recorder.AppendData(stereo + 2, 2);
        require(recorder.stopRecording(), "finalize stereo recording");
        verifyWav(first, 44100, 2, {stereo, stereo + 6});
        recorder.AppendData(stereo, 3);
        require(recorder.stopRecording(), "repeated stop");
        verifyWav(first, 44100, 2, {stereo, stereo + 6});

        const auto second = temporary.path / "mono.wav";
        require(recorder.startRecording(second.string().c_str(), 48000, 1), "restart mono recording");
        const float mono[] = {-0.75f, 0.125f};
        recorder.AppendData(mono, 2);
        require(recorder.stopRecording(), "finalize mono recording");
        verifyWav(second, 48000, 1, {mono, mono + 2});

        const auto missing = temporary.path / "missing" / "failed.wav";
        require(!recorder.startRecording(missing.string().c_str()), "failed open reported");
        recorder.AppendData(stereo, 3);
        require(recorder.stopRecording(), "stop after failed start");
        require(!fs::exists(missing), "failed open creates no file");

        const auto empty = temporary.path / "empty.wav";
        require(recorder.startRecording(empty.string().c_str()), "recover after failed start");
        require(recorder.stopRecording(), "finalize empty recording");
        verifyWav(empty, 44100, 2, {});

        const auto active = temporary.path / "active.wav";
        const auto restarted = temporary.path / "restarted.wav";
        require(recorder.startRecording(active.string().c_str()), "start before active restart");
        recorder.AppendData(stereo, 3);
        require(recorder.startRecording(restarted.string().c_str()), "restart active recording");
        verifyWav(active, 44100, 2, {stereo, stereo + 6});
        recorder.AppendData(stereo, 1);
        require(recorder.stopRecording(), "finalize restarted recording");
        verifyWav(restarted, 44100, 2, {stereo, stereo + 2});

        const auto large = temporary.path / "large.wav";
        std::vector<float> largeSamples(10006);
        for (std::size_t i = 0; i < largeSamples.size(); ++i)
            largeSamples[i] = static_cast<float>(static_cast<int>(i % 2001) - 1000) / 1024.0f;
        require(recorder.startRecording(large.string().c_str()), "start multi-block recording");
        recorder.AppendData(largeSamples.data(), largeSamples.size() / 2);
        require(recorder.stopRecording(), "finalize multi-block recording with partial final block");
        verifyWav(large, 44100, 2, largeSamples);

        const auto automatic = temporary.path / "destructor.wav";
        {
            WavRecorder scopedRecorder;
            require(scopedRecorder.startRecording(automatic.string().c_str()), "start scoped recording");
            scopedRecorder.AppendData(stereo, 3);
        }
        verifyWav(automatic, 44100, 2, {stereo, stereo + 6});

        const auto invalid = temporary.path / "invalid.wav";
        require(!recorder.startRecording(invalid.string().c_str(), 0, 2), "reject zero sample rate");
        require(!recorder.startRecording(invalid.string().c_str(), 44100, 0), "reject zero channels");
        require(!fs::exists(invalid), "invalid format creates no file");
    } catch (const std::exception& error) {
        std::cerr << "WavRecorder regression failure: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
