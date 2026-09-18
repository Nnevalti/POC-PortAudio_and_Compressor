#include <iostream>

#include "StreamHandler.h"

StreamHandler::StreamHandler()
{
    err = Pa_Initialize();
    if( err != paNoError )
        handleError();
}

StreamHandler::~StreamHandler()
{
    closeStream();
    Pa_Terminate();
}

bool StreamHandler::initInput(PaDeviceIndex deviceId)
{
    if (deviceId == paNoDevice)
        deviceId = Pa_GetDefaultInputDevice();

    if (deviceId == paNoDevice) {
        std::cerr << "No default input device is available." << std::endl;
        return false;
    }

    inputParameters.device = deviceId;
    inputInfo = Pa_GetDeviceInfo( inputParameters.device );
    if (inputInfo == nullptr || inputInfo->maxInputChannels <= 0) {
        std::cerr << "Selected input device cannot capture audio." << std::endl;
        return false;
    }

    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = inputInfo->defaultHighInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    return true;
}

bool StreamHandler::initOutput(PaDeviceIndex deviceId)
{
    if (deviceId == paNoDevice)
        deviceId = Pa_GetDefaultOutputDevice();

    if (deviceId == paNoDevice) {
        std::cerr << "No default output device is available." << std::endl;
        return false;
    }

    outputParameters.device = deviceId;
    outputInfo = Pa_GetDeviceInfo(outputParameters.device);
    if (outputInfo == nullptr || outputInfo->maxOutputChannels <= 0) {
        std::cerr << "Selected output device cannot play audio." << std::endl;
        return false;
    }

    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = outputInfo->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    return true;
}

bool StreamHandler::configureStereoChannels()
{
    if (inputInfo == nullptr || outputInfo == nullptr)
        return false;

    constexpr int channelCount = 2;
    if (inputInfo->maxInputChannels < channelCount || outputInfo->maxOutputChannels < channelCount) {
        std::cerr << "This proof of concept requires stereo input and output devices." << std::endl;
        return false;
    }

    inputParameters.channelCount = channelCount;
    outputParameters.channelCount = channelCount;
    err = Pa_IsFormatSupported(&inputParameters, &outputParameters, SAMPLE_RATE);
    if (err != paFormatIsSupported) {
        displayPaError();
        return false;
    }
    return true;
}

bool StreamHandler::openStream(PaStreamCallback callback)
{
    err = Pa_OpenStream(
            &stream,
            &inputParameters,
            &outputParameters,
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            0,      /* we won't output out of range samples so don't bother clipping them */
            callback,
            &data); /* no callback, so no callback userData */

    if( err != paNoError ) {
        displayPaError();
        return false;
    }
    if (!data.recorder.startRecording("original_audio.wav", SAMPLE_RATE,
                                      inputParameters.channelCount)) {
        std::cerr << "Could not start input WAV recording." << std::endl;
        closeStream();
        return false;
    }
    return true;
}

bool StreamHandler::startStream()
{
    err = Pa_StartStream( stream );
    if( err != paNoError ) {
        displayPaError();
        return false;
    }
    return true;
}

bool StreamHandler::stopStream()
{
    err = Pa_StopStream( stream );
    if( err != paNoError ) {
        displayPaError();
        return false;
    }

    const bool originalSaved = data.recorder.stopRecording();
    const bool compressedSaved = data.compressor.recorder.stopRecording();
    if (!originalSaved || !compressedSaved)
        std::cerr << "Could not save a complete WAV recording." << std::endl;
    return originalSaved && compressedSaved;
}

void StreamHandler::closeStream()
{
    if( stream )
    {
        Pa_AbortStream( stream );
        Pa_CloseStream( stream );
        stream = nullptr;
    }
    data.recorder.stopRecording();
    data.compressor.recorder.stopRecording();
}

void StreamHandler::handleError()
{
    closeStream();
    Pa_Terminate();
    displayPaError();
}

void StreamHandler::displayPaError()
{
    std::cerr << "An error occurred while using the portaudio stream" << std::endl;
    std::cerr << "Error number: " << err << std::endl;
    std::cerr << "Error message: " << Pa_GetErrorText(err) << std::endl;
}
