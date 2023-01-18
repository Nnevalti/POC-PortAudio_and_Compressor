/**
 * Author:              Valentin Deschamps
 * Modification by :    Valentin Deschamps
 * Created:             20/10/2022
 * Modified:            20/10/2022
 * 
 * © Copyright by SoundX
 **/

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
    Pa_Terminate();
}

void StreamHandler::initInput(unsigned int deviceId)
{
    inputParameters.device = deviceId;
    inputInfo = Pa_GetDeviceInfo( inputParameters.device );
    // For now it only works if both devices has the same number of channels
    // Maybe define something to choose when init, because callback must behave accordingly to this param
    inputParameters.channelCount = inputInfo->maxInputChannels;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = inputInfo->defaultHighInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
}

void StreamHandler::initOutput(unsigned int deviceId)
{
    outputParameters.device = deviceId;
    outputInfo = Pa_GetDeviceInfo(outputParameters.device);
    // For now it only works if both devices has the same number of channels
    // Maybe define something to choose when init, because callback must behave accordingly to this param
    outputParameters.channelCount = outputInfo->maxOutputChannels;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = outputInfo->defaultHighInputLatency ;
    outputParameters.hostApiSpecificStreamInfo = NULL;
}

void StreamHandler::openStream(PaStreamCallback callback)
{
    err = Pa_OpenStream(
            &stream,
            &inputParameters,
            &outputParameters,
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            0,      /* we won't output out of range samples so don't bother clipping them */
            callback,
            &recorder ); /* no callback, so no callback userData */

    if( err != paNoError )
        handleError();
}

void StreamHandler::startStream()
{
    err = Pa_StartStream( stream );
    if( err != paNoError )
        handleError();
}

void StreamHandler::stopStream()
{
    err = Pa_StopStream( stream );
    if( err != paNoError )
        handleError();
}

void StreamHandler::closeStream()
{
    if( stream )
    {
        Pa_AbortStream( stream );
        Pa_CloseStream( stream );
    }
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