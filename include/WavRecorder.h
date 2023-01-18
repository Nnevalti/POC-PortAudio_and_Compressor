#include <vector>
#include <fstream>
#include <iostream>

class WavRecorder {


  typedef struct WAV_HEADER {
    /* RIFF Chunk Descriptor */
    uint8_t RIFF[4] = {'R', 'I', 'F', 'F'}; // RIFF Header Magic header
    uint32_t ChunkSize;                     // RIFF Chunk Size = 36 + subchunk2size
    uint8_t WAVE[4] = {'W', 'A', 'V', 'E'}; // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t fmt[4] = {'f', 'm', 't', ' '}; // FMT header
    uint32_t Subchunk1Size = 16;           // Size of the fmt chunk
    uint16_t AudioFormat = 3; // Audio format 1=PCM,  3=IEEE float, 6=mulaw, 7=alaw, 257=IBM
                              // Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t NumOfChan = 2;   // Number of channels 1=Mono 2=Stereo
    uint32_t SampleRate = 44100;   // Sampling Frequency in Hz
    uint32_t ByteRate = SampleRate * NumOfChan * 4; // bytes per second (4 = sizeof(float)/8)
    uint16_t blockAlign = NumOfChan * 4;          // (4 = sizeof(float)/8)
    uint16_t bitsPerSample = 32;      // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t Subchunk2ID[4] = {'d', 'a', 't', 'a'}; // "data"  string
    uint32_t Subchunk2Size = 0;                        // Sampled data length
  } wav_hdr;

public:
    WAV_HEADER  header;
    std::ofstream file;
    bool isRecording;
    std::vector<float> data_buffer;

    WavRecorder();

    void startRecording(const char* fileName);
    void AppendData(const float *data, unsigned long nbFrames);
    void stopRecording();
    void print_header();
};
