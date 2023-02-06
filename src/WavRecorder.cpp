#include "WavRecorder.h"

WavRecorder::WavRecorder() {}

void WavRecorder::startRecording(const char* fileName) {
    file.open(fileName, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "Error: Could not open file for writing." << std::endl;
        return;
    }

    isRecording = true;
}

void WavRecorder::AppendData(const float* data, unsigned long nbFrames) {
    for (int i = 0; i < nbFrames; i++)
    {
        for (int c = 0; c < header.NumOfChan; c++) {
            data_buffer.push_back(data[i * header.NumOfChan + c]);
        }
    }
    header.Subchunk2Size += nbFrames * header.NumOfChan * sizeof(float); // NumSamples * NumChannels * BitsPerSample/8
}

void WavRecorder::print_header() {
    std::cout << "RIFF : " << header.RIFF << std::endl;
    std::cout << "ChunkSize : " << header.ChunkSize << std::endl;
    std::cout << "WAVE : " << header.WAVE << std::endl;
    std::cout << "fmt : " << header.fmt << std::endl;
    std::cout << "SubCHunk1Size : " << header.Subchunk1Size << std::endl;
    std::cout << "Audio Format : " << header.AudioFormat << std::endl;
    std::cout << "Number of channels : " << header.NumOfChan << std::endl;
    std::cout << "Sample Rate : " << header.SampleRate << std::endl;
    std::cout << "Byte Rate : " << header.ByteRate << std::endl;
    std::cout << "Block Align : " << header.blockAlign << std::endl;
    std::cout << "Bits per sample : " << header.bitsPerSample << std::endl;
    std::cout << "data : " << header.Subchunk2ID << std::endl;
    std::cout << "SubChunk2Size : " << header.Subchunk2Size << std::endl;
    std::cout << "Data : " << std::endl;
    // for (auto i : data_buffer) {
    //     std::cout << i << std::endl;
    // }
}

void WavRecorder::stopRecording() {

    header.ChunkSize = 36 + header.Subchunk2Size;
    file.write(reinterpret_cast<char *>(&header), sizeof(WAV_HEADER));
    file.write((char *)data_buffer.data(), data_buffer.size() * sizeof(float));

    isRecording = false;
    // print_header();
    file.close();
}
