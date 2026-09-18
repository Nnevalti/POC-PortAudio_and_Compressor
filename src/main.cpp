#include "StreamHandler.h"
#include "Compressor.h"
#include <cstring>
#include <iostream>

int callback   (const void *input,
                void *output,
                unsigned long frameCount,
                const PaStreamCallbackTimeInfo *timeInfo,
                PaStreamCallbackFlags statusFlags,
                void *userData)
{
    t_data *data = (t_data*)userData;

    // If recording is on, append data to buffer
    if (data->recorder.isRecording) {
        const float* in = (const float*)input;
        data->recorder.AppendData(in, frameCount);
    }

    const unsigned int nbChannels = static_cast<unsigned int>(data->recorder.header.NumOfChan);
    float *compressed_input;

    // Apply compressor to input signal and store it in output
    compressed_input = data->compressor.Compress((const float*)input, frameCount, nbChannels);

    std::memcpy(output, compressed_input, frameCount * nbChannels * SAMPLE_SIZE);
    // std::memcpy(output, input, frameCount * nbChannels * SAMPLE_SIZE);

    /* Check return value at http://files.portaudio.com/docs/v19-doxydocs/portaudio_8h.html#ae9bfb9c4e1895326f30f80d415c66c32 */
    return paContinue; 
}

int main()
{
    freopen("error.log", "w", stderr);

    StreamHandler stream;

    if (!stream.initInput() || !stream.initOutput() || !stream.configureStereoChannels() ||
        !stream.openStream(&callback) || !stream.startStream()) {
        return 1;
    }

    char input;
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << FRAMES_PER_BUFFER << ").\n";
    std::cin.get(input);

    if (!stream.stopStream())
        return 1;
    stream.closeStream();

    return 0;
}
