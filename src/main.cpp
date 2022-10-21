#include <iostream>
#include <string>
#include "portaudio.h"

/* #define SAMPLE_RATE  (17932) // Test failure to open with this value. */
#define SAMPLE_RATE       (44100)
#define FRAMES_PER_BUFFER   (16)

#define NUM_SECONDS          (10000)
/* #define DITHER_FLAG     (paDitherOff)  */
// #define DITHER_FLAG           (0)

/* Select sample format. */
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

int callback(const void *input,
            void *output,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo *timeInfo,
            PaStreamCallbackFlags statusFlags,
            void *userData)
{
    unsigned int nbChannels = 2;

    std::memcpy(output, input, frameCount * nbChannels * SAMPLE_SIZE);

    /* Check return value at http://files.portaudio.com/docs/v19-doxydocs/portaudio_8h.html#ae9bfb9c4e1895326f30f80d415c66c32 */
    return paContinue; 
}

int main(void)
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream = NULL;
    PaError err;
    const PaDeviceInfo* inputInfo;
    const PaDeviceInfo* outputInfo;

    int numChannels;

    std::cout << "Test: PortAudio input->output" << std::endl;
    std::cout << "sizeof(int) = " << sizeof(int) << std::endl;
    std::cout << "sizeof(long) = " << sizeof(long) << std::endl;

/* INITILIZE STREAM */
    err = Pa_Initialize();
    if( err != paNoError )
    {
        if( stream ) {
            Pa_AbortStream( stream );
            Pa_CloseStream( stream );
        }
        Pa_Terminate();
        std::cerr << "An error occurred while using the portaudio stream" << std::endl;
        std::cerr << "Error number: " << err << std::endl;
        std::cerr << "Error message: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

/* INPUT DEVICE */
    inputParameters.device = 1; // input device id
    inputInfo = Pa_GetDeviceInfo( inputParameters.device );
    std::cout << "Input device # " << inputParameters.device << std::endl;
    std::cout << "        Name: " << inputInfo->name << std::endl;
    std::cout << " Low Latency: " << inputInfo->defaultLowOutputLatency << std::endl;
    std::cout << "High Latency: " << inputInfo->defaultHighOutputLatency << std::endl;

/* OUTPUT DEVICE */
    outputParameters.device = 2; /* default output device */
    outputInfo = Pa_GetDeviceInfo( outputParameters.device );
    std::cout << "Output device # " << outputParameters.device << std::endl;
    std::cout << "        Name: " << outputInfo->name << std::endl;
    std::cout << " Low Latency: " << outputInfo->defaultLowOutputLatency << std::endl;
    std::cout << "High Latency: " << outputInfo->defaultHighOutputLatency << std::endl;

/* Number of minimum channel */
    numChannels = inputInfo->maxInputChannels < outputInfo->maxOutputChannels
            ? inputInfo->maxInputChannels : outputInfo->maxOutputChannels;
    std::cout << "Min Num channels (between input and output) = " << numChannels << std::endl;

/* INPUT PARAMETERS */
    inputParameters.channelCount = numChannels;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = inputInfo->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

/* OUTPUT PARAMETERS */
    outputParameters.channelCount = numChannels;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = outputInfo->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

/* OPEN STREAM */
    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              0,      /* we won't output out of range samples so don't bother clipping them */
              &callback, /* no callback, use blocking API */
              NULL ); /* no callback, so no callback userData */

    if( err != paNoError )
    {
        if( stream ) {
            Pa_AbortStream( stream );
            Pa_CloseStream( stream );
        }
        Pa_Terminate();
        std::cerr << "An error occurred while using the portaudio stream" << std::endl;
        std::cerr << "Error number: " << err << std::endl;
        std::cerr << "Error message: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

/* START STREAM */
    err = Pa_StartStream( stream );
    if( err != paNoError )
    {
        if( stream ) {
            Pa_AbortStream( stream );
            Pa_CloseStream( stream );
        }
        Pa_Terminate();
        std::cerr << "An error occurred while using the portaudio stream" << std::endl;
        std::cerr << "Error number: " << err << std::endl;
        std::cerr << "Error message: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

/* WAIT FOR USER TO STOP STREAM */
    char input;
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << FRAMES_PER_BUFFER << ").\n";
    std::cin.get(input);

/* CLEAN EVERYTHING AND LEAVE */
    err = Pa_StopStream( stream );
    if( err != paNoError )
    {
        if( stream ) {
            Pa_AbortStream( stream );
            Pa_CloseStream( stream );
        }
        Pa_Terminate();
        std::cerr << "An error occurred while using the portaudio stream" << std::endl;
        std::cerr << "Error number: " << err << std::endl;
        std::cerr << "Error message: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    Pa_Terminate();
    return 0;
}
