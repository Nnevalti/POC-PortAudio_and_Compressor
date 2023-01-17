#include "StreamHandler.h"
#include <iostream>
#include <iostream>
#include <fstream>
#include <vector>

int callback   (const void *input,
                void *output,
                unsigned long frameCount,
                const PaStreamCallbackTimeInfo *timeInfo,
                PaStreamCallbackFlags statusFlags,
                void *userData)
{
    WavCreator* creator = (WavCreator*)userData;
    if (creator->isRecording) {
        const float* in = (const float*)input;
        creator->AppendData(in, frameCount);
        // creator->file.write((char*)in, frameCount * creator->numChannels * sizeof(float));
    }
    
    std::memcpy(output, input, frameCount * 2 * SAMPLE_SIZE);

    /* Check return value at http://files.portaudio.com/docs/v19-doxydocs/portaudio_8h.html#ae9bfb9c4e1895326f30f80d415c66c32 */
    return paContinue; 
}

int main()
{
    StreamHandler stream;

    stream.initInput(2);
    stream.initOutput(4);
    stream.openStream(&callback);
    stream.startStream();

    stream.creator.startRecording("audio.wav");

    char input;
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << FRAMES_PER_BUFFER << ").\n";
    std::cin.get(input);

    stream.creator.stopRecording();    
    stream.stopStream();

    return 0;
}
