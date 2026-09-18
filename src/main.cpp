#include "StreamHandler.h"
#include "Compressor.h"
#include <iostream>
#include <algorithm>
#include <cstddef>

int callback   (const void *input,
                void *output,
                unsigned long frameCount,
                const PaStreamCallbackTimeInfo *,
                PaStreamCallbackFlags,
                void *userData)
{
    t_data *data = (t_data*)userData;

    const unsigned int nbChannels = static_cast<unsigned int>(data->recorder.header.NumOfChan);
    const float* in = static_cast<const float*>(input);
    float* out = static_cast<float*>(output);

    if (in == nullptr) {
        // Keep both recordings aligned when capture supplies a silent block.
        std::fill_n(out, static_cast<std::size_t>(frameCount) * nbChannels, 0.0f);
        in = out;
    }
    data->recorder.AppendData(in, frameCount);
    // PortAudio owns the output buffer; processing needs no temporary allocation.
    data->compressor.process(in, out, frameCount, nbChannels);

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
