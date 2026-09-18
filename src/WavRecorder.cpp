#include "WavRecorder.h"

#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>

namespace {
constexpr uint32_t riffOverhead = 50;

void writeLE(std::ofstream& file, uint32_t value, unsigned bytes) {
    char encoded[4];
    for (unsigned i = 0; i < bytes; ++i) {
        encoded[i] = static_cast<char>((value >> (8 * i)) & 0xff);
    }
    file.write(encoded, bytes);
}
}

WavRecorder::~WavRecorder() {
    stopRecording();
}

bool WavRecorder::startRecording(const char* fileName, uint32_t sampleRate,
                                 uint16_t channels) {
    if (!stopRecording()) {
        return false;
    }
    data_buffer.clear();
    header = WAV_HEADER{};
    recordingFailed = false;
    file.clear();

    const uint64_t blockAlign = static_cast<uint64_t>(channels) * 4;
    if (!fileName || !*fileName || sampleRate == 0 || channels == 0 ||
        blockAlign > std::numeric_limits<uint16_t>::max() ||
        sampleRate * blockAlign > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    header.NumOfChan = channels;
    header.SampleRate = sampleRate;
    header.blockAlign = static_cast<uint16_t>(blockAlign);
    header.ByteRate = static_cast<uint32_t>(sampleRate * blockAlign);
    file.open(fileName, std::ios::binary | std::ios::trunc);
    isRecording = file.is_open() && file.good();
    return isRecording;
}

void WavRecorder::AppendData(const float* data, unsigned long nbFrames) {
    if (!isRecording || recordingFailed || !data || nbFrames == 0) {
        return;
    }
    // Classic RIFF is limited to 32-bit chunk sizes. Reject a whole callback
    // rather than overflow the header or retain a partial audio frame.
    const uint64_t available = std::numeric_limits<uint32_t>::max() -
                               riffOverhead - header.Subchunk2Size;
    if (nbFrames > available / header.blockAlign) {
        recordingFailed = true;
        return;
    }
    const size_t sampleCount = static_cast<size_t>(nbFrames) * header.NumOfChan;
    try {
        data_buffer.insert(data_buffer.end(), data, data + sampleCount);
    } catch (const std::bad_alloc&) {
        recordingFailed = true;
        return;
    } catch (const std::length_error&) {
        recordingFailed = true;
        return;
    }
    header.Subchunk2Size += static_cast<uint32_t>(sampleCount * sizeof(float));
    header.ChunkSize = riffOverhead + header.Subchunk2Size;
}

bool WavRecorder::stopRecording() {
    if (!isRecording) {
        return true;
    }
    isRecording = false;
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                  "WAV recording requires 32-bit IEEE floats");
    // Serialize fields explicitly: struct padding and host endianness must not
    // affect the file. Non-PCM WAV includes a fmt extension and fact chunk.
    file.write("RIFF", 4);
    writeLE(file, header.ChunkSize, 4);
    file.write("WAVEfmt ", 8);
    writeLE(file, 18, 4);
    writeLE(file, 3, 2);
    writeLE(file, header.NumOfChan, 2);
    writeLE(file, header.SampleRate, 4);
    writeLE(file, header.ByteRate, 4);
    writeLE(file, header.blockAlign, 2);
    writeLE(file, 32, 2);
    writeLE(file, 0, 2); // cbSize: no format-specific extension.
    file.write("fact", 4);
    writeLE(file, 4, 4);
    writeLE(file, header.Subchunk2Size / header.blockAlign, 4);
    file.write("data", 4);
    writeLE(file, header.Subchunk2Size, 4);
    std::array<char, 16384> encoded;
    size_t used = 0;
    for (float sample : data_buffer) {
        uint32_t bits;
        std::memcpy(&bits, &sample, sizeof(bits));
        for (unsigned byte = 0; byte < 4; ++byte) {
            encoded[used++] = static_cast<char>((bits >> (8 * byte)) & 0xff);
        }
        if (used == encoded.size()) {
            file.write(encoded.data(), static_cast<std::streamsize>(used));
            used = 0;
            if (!file) {
                break;
            }
        }
    }
    if (used != 0) {
        file.write(encoded.data(), static_cast<std::streamsize>(used));
    }
    file.close();
    return !recordingFailed && !file.fail();
}

void WavRecorder::print_header() {
    std::cout << "RIFF : ";
    std::cout.write(reinterpret_cast<const char*>(header.RIFF), 4);
    std::cout << "\nChunkSize : " << header.ChunkSize << "\nWAVE : ";
    std::cout.write(reinterpret_cast<const char*>(header.WAVE), 4);
    std::cout << "\nfmt : ";
    std::cout.write(reinterpret_cast<const char*>(header.fmt), 4);
    std::cout << "\nSubChunk1Size : " << header.Subchunk1Size
              << "\nAudio Format : " << header.AudioFormat
              << "\nNumber of channels : " << header.NumOfChan
              << "\nSample Rate : " << header.SampleRate
              << "\nByte Rate : " << header.ByteRate
              << "\nBlock Align : " << header.blockAlign
              << "\nBits per sample : " << header.bitsPerSample << "\ndata : ";
    std::cout.write(reinterpret_cast<const char*>(header.Subchunk2ID), 4);
    std::cout << "\nSubChunk2Size : " << header.Subchunk2Size << '\n';
}
