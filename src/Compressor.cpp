#include "Compressor.h"
#include <cmath>
#include <cstring>
#include <iostream>

Compressor::Compressor()
{
	sampleRate = 44100;

	threshold = -20.0f;
	knee = 5.0f;
	ratio = 2.0f;
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

float *Compressor::Compress(const float* input, const unsigned int nbFrames, const unsigned int nbChannels)
{
    float *output = new float[nbFrames * nbChannels];
	float threshold_lin = db2lin(threshold); // convert the threshold to linear
	float kneeWidth = db2lin(knee); // convert the knee to linear
	float kneeStart = db2lin(threshold - knee/2); // get the knee start in linear
	float kneeEnd = db2lin(threshold + knee/2); // get the knee end in linear

	std::memset(output, 0, nbFrames * nbChannels * sizeof(float));

	for (unsigned int i = 0; i < nbFrames; i++) 
	{
		for (unsigned int j = 0; j < nbChannels; j++)
		{
			float level = fabs(input[i * nbChannels + j]);

			if (level <= kneeStart) 
			{
				output[i * nbChannels + j] = input[i * nbChannels + j];
				std::cerr << "level <= kneeStart" << std::endl;
			}
			else if (level <= kneeEnd)
			{
				float gainReduction = (level - kneeStart) / kneeWidth * (threshold_lin - kneeStart) / ratio;
            	output[i * nbChannels + j] = level - gainReduction;
				std::cerr << "level <= kneeEnd" << std::endl;
			}
			else 
			{
				output[i * nbChannels + j] = threshold_lin + ((level - threshold_lin) / ratio);
			}
			if (input[i * nbChannels + j] < 0 && output[i * nbChannels + j] > 0) 
			{
				output[i * nbChannels + j] *= -1;
			}
			// output[i * nbChannels + j] *= db2lin(makeUpGain);
		}
	}

	recorder.AppendData(output, nbFrames);
    return output;
}