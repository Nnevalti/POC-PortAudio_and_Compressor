#include "Compressor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

Compressor::Compressor(bool recordOutput)
{
	sampleRate = 44100;

	threshold = -20.0f;
	knee = 0.0f;
	ratio = 5.0f;
	makeUpGain = 3.0f;

	this->recordOutput = recordOutput;
	if (recordOutput)
		recorder.startRecording("compressed_audio.wav");
}

Compressor::~Compressor()
{
	if (recordOutput)
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
	const float kneeWidth = std::max(0.0f, knee);

	std::memset(output, 0, nbFrames * nbChannels * sizeof(float));

	for (unsigned int i = 0; i < nbFrames; i++)
	{
		for (unsigned int j = 0; j < nbChannels; j++)
		{
			float level = fabs(input[i * nbChannels + j]);

			if (level == 0.0f) {
				output[i * nbChannels + j] = 0.0f;
			}
			else {
				const float inputDb = lin2db(level);
				float gainReductionDb = 0.0f;

				if (kneeWidth <= 0.0f) {
					if (inputDb > threshold)
						gainReductionDb = (1.0f / ratio - 1.0f) * (inputDb - threshold);
				}
				else if (inputDb < threshold - kneeWidth / 2.0f) {
					gainReductionDb = 0.0f;
				}
				else if (inputDb <= threshold + kneeWidth / 2.0f) {
					const float distanceIntoKnee = inputDb - threshold + kneeWidth / 2.0f;
					gainReductionDb =
						(1.0f / ratio - 1.0f) * distanceIntoKnee * distanceIntoKnee /
						(2.0f * kneeWidth);
				}
				else {
					gainReductionDb = (1.0f / ratio - 1.0f) * (inputDb - threshold);
				}

				output[i * nbChannels + j] = db2lin(inputDb + gainReductionDb);
			}

			if (input[i * nbChannels + j] < 0)
				output[i * nbChannels + j] *= -1;

			// Apply the make-up gain
			output[i * nbChannels + j] *= db2lin(makeUpGain);
		}
	}

	if (recordOutput)
		recorder.AppendData(output, nbFrames);
    return output;
}
