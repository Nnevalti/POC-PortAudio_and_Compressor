/**
 * Author:              Valentin Deschamps
 * Modification by :    Valentin Deschamps
 * Created:             20/10/2022
 * Modified:            20/10/2022
 * 
 * © Copyright by SoundX
 **/

#pragma once

#include "portaudio.h"
#include "WavCreator.h"

/**
 * StreamHandler Class (temporary name)
 * 
 * This class aims to handle the different function of the PortAudio library
 * It will handle the input and output devices as well as the different stream parameters.
 * This file will also define constants such as sample rate and frames per buffer
 **/

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 16

/* SAMPLE FORMAT */
#if 1
#define PA_SAMPLE_TYPE  paFloat32
#define SAMPLE_SIZE (4)
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 0
#define PA_SAMPLE_TYPE  paInt16
#define SAMPLE_SIZE (2)
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt24
#define SAMPLE_SIZE (3)
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
#define SAMPLE_SIZE (1)
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
#define SAMPLE_SIZE (1)
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif

class WavCreator;

class StreamHandler
{
    public:
        StreamHandler();
        ~StreamHandler();
        // void init(); // Will probably do that in constructor
        void initInput(unsigned int deviceId); // Setup Input Params
        void initOutput(unsigned int deviceId); // Setup Output Params
        void openStream(PaStreamCallback callback);
        void startStream();
        void stopStream();
        void closeStream();
        void handleError();
        void displayPaError();
        WavCreator creator;

    private:
        PaStreamParameters inputParameters, outputParameters;
        PaStream *stream;
        PaError err;
        const PaDeviceInfo* inputInfo;
        const PaDeviceInfo* outputInfo;

        // unsigned int numChannels;
        
};