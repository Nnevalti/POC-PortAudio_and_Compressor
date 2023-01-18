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
    WavRecorder* recorder = (WavRecorder*)userData;
    if (recorder->isRecording) {
        const float* in = (const float*)input;
        recorder->AppendData(in, frameCount);
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

    stream.recorder.startRecording("audio.wav");

    char input;
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << FRAMES_PER_BUFFER << ").\n";
    std::cin.get(input);

    stream.recorder.stopRecording();    
    stream.stopStream();

    return 0;
}
