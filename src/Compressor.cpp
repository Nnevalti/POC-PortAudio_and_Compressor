#include "Compressor.h"
#include <cmath>
#include <cstring>
#include <iostream>

Compressor::Compressor()
{
	sampleRate = 44100;

	threshold = -20.0f;
	knee = 0.0f;
	ratio = 5.0f;
	makeUpGain = 3.0f;

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

inline float Compressor::kneeWidth2lin(float knee){ // knee width to linear
	if (knee == 0)
		return 0;
	return powf(10.0f, 0.05f * knee); // 0,05 = 1/20
}

float *Compressor::Compress(const float* input, const unsigned int nbFrames, const unsigned int nbChannels)
{
	// linear
    float *output = new float[nbFrames * nbChannels];
	float threshold_lin = db2lin(threshold); // convert the threshold to linear
	float kneeWidth = kneeWidth2lin(-knee); // convert the knee to linear

	std::memset(output, 0, nbFrames * nbChannels * sizeof(float));

	for (unsigned int i = 0; i < nbFrames; i++)
	{
		for (unsigned int j = 0; j < nbChannels; j++)
		{
			float level = fabs(input[i * nbChannels + j]);

			// Knee compression algorithm
			if (level - threshold_lin < -kneeWidth/2) {
				output[i * nbChannels + j] = level;
			}
			else if (fabs(level - threshold_lin) <= kneeWidth/2) {
				output[i * nbChannels + j] = level + ((1/ratio - 1) * pow(level - threshold_lin + kneeWidth/2, 2)) / (2 * kneeWidth);
			}
			else if (level - threshold_lin > kneeWidth/2) {
				output[i * nbChannels + j] = threshold_lin + ((level - threshold_lin) / ratio);
			}

			if (input[i * nbChannels + j] < 0)
				output[i * nbChannels + j] *= -1;

			// Apply the make-up gain
			output[i * nbChannels + j] *= db2lin(makeUpGain);
		}
	}

	recorder.AppendData(output, nbFrames);
    return output;
}