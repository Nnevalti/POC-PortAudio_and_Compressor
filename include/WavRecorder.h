#pragma once

#include <cstdint>
#include <fstream>
#include <vector>

// Buffers interleaved IEEE float samples in memory until stopRecording().
// Calls must be externally synchronized; stop only after the audio callback stops.
class WavRecorder {
public:
    struct WAV_HEADER {
        uint8_t RIFF[4] = {'R', 'I', 'F', 'F'};
        uint32_t ChunkSize = 50; // Includes fmt extension and fact chunk.
        uint8_t WAVE[4] = {'W', 'A', 'V', 'E'};
        uint8_t fmt[4] = {'f', 'm', 't', ' '};
        uint32_t Subchunk1Size = 18;
        uint16_t AudioFormat = 3;
        uint16_t NumOfChan = 2;
        uint32_t SampleRate = 44100;
        uint32_t ByteRate = 44100 * 2 * 4;
        uint16_t blockAlign = 2 * 4;
        uint16_t bitsPerSample = 32;
        uint8_t Subchunk2ID[4] = {'d', 'a', 't', 'a'};
        uint32_t Subchunk2Size = 0;
    };

    WAV_HEADER header;
    std::ofstream file;
    bool isRecording = false;
    std::vector<float> data_buffer;

    WavRecorder() = default;
    ~WavRecorder();

    bool startRecording(const char* fileName, uint32_t sampleRate = 44100,
                        uint16_t channels = 2);
    void AppendData(const float* data, unsigned long nbFrames);
    bool stopRecording();
    void print_header();

private:
    bool recordingFailed = false;
};
