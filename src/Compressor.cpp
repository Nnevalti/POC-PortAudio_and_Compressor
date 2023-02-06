#include "Compressor.h"
#include <cmath>
#include <cstring>
#include <iostream>

Compressor::Compressor()
{
	sampleRate = 44100;

	threshold = -20.0f;
	ratio = 8.0f;
	makeUpGain = 6.0f;

	recorder.startRecording("compressed_audio.wav");
}

Compressor::~Compressor()
{
	recorder.stopRecording();
}

inline float Compressor::db2lin(float db){ // dB to linear
	return powf(10.0f, 0.05f * db); // 0,05 = 1/20
}

inline float Compressor::lin2db(float lin){ // linear to dB
	return 20.0f * log10f(lin);
}

float *Compressor::Compress(const float* input, const unsigned int nbFrames, const unsigned int nbChannels)
{
    float *output = new float[nbFrames * nbChannels];
	std::memset(output, 0, nbFrames * nbChannels * sizeof(float));

	for (unsigned int i = 0; i < nbFrames; i++) // 0 -> 15
	{
		for (unsigned int j = 0; j < nbChannels; j++) // 0 -> 1
		{
			float level = fabs(input[i * nbChannels + j]); // get absolute value of the input
			float threshold_lin = db2lin(threshold); // convert the threshold to linear
			if (level > threshold_lin) {
				output[i * nbChannels + j] = threshold_lin + ((level - threshold_lin) / ratio); // apply the ratio
				if (input[i * nbChannels + j] < 0) // if the input is negative, make the output negative
					output[i * nbChannels + j] *= -1;
				// std::cout << "compressing " << i * nbChannels + j << std::endl;
				// std::cout << input[i * nbChannels + j] << " , " << output[i * nbChannels + j] << std::endl;
				// std::cout << "threshold_lin: " << threshold_lin << " , " << "level: " << level << std::endl;
			}
			else {
				output[i * nbChannels + j] = input[i * nbChannels + j]; // no compression
			}
			output[i * nbChannels + j] *= db2lin(makeUpGain); // apply the make up gain
		}
	}
	recorder.AppendData(output, nbFrames);
    return output;
}

// float *Compressor::Compress(const float* input, const unsigned int nbFrames, const unsigned int nbChannels)
// {
//     float *output = new float[nbFrames * nbChannels];
// 	std::memset(output, 0, nbFrames * nbChannels * sizeof(float));
// 	float gain = 1.0;
// 	float attackCoeff = exp(-1.0 / (attack * sampleRate));
// 	float releaseCoeff = exp(-1.0 / (release * sampleRate));

// 	for (unsigned int i = 0; i < nbFrames; i++)
// 	{
// 		for (unsigned int j = 0; j < nbChannels; j++)
// 		{
// 			float level = fabs(input[i * nbChannels + j]);
// 			float threshold_lin = db2lin(threshold);
// 			if (level > threshold_lin) {
// 				gain = threshold_lin + ((level - threshold_lin) / ratio);
// 			}
// 			else {
// 				gain = level;
// 			}

// 			// Attack and Release
// 			if (gain > envelope) {
// 				envelope = attackCoeff * (envelope - gain) + gain;
// 			} else {
// 				envelope = releaseCoeff * (envelope - gain) + gain;
// 			}

// 			output[i * nbChannels + j] = input[i * nbChannels + j] * (1.0 / envelope);
// 		}
// 	}
//     return output;
// }