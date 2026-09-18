#include "Compressor.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

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
    float *output = new float[static_cast<std::size_t>(nbFrames) * nbChannels];
    process(input, output, nbFrames, nbChannels);
    return output;
}

void Compressor::process(const float* input, float* output, unsigned int nbFrames, unsigned int nbChannels)
{
    if (output == nullptr || nbFrames == 0 || nbChannels == 0) {
        return;
    }
	const float kneeWidth = std::max(0.0f, knee);

	for (unsigned int i = 0; i < nbFrames; i++)
	{
		for (unsigned int j = 0; j < nbChannels; j++)
		{
			const std::size_t index = static_cast<std::size_t>(i) * nbChannels + j;
			const float sample = input ? input[index] : 0.0f;
			float level = fabs(sample);

			if (level == 0.0f) {
				output[index] = 0.0f;
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

				output[index] = db2lin(inputDb + gainReductionDb);
			}

			if (sample < 0)
				output[index] *= -1;

			// Apply the make-up gain
			output[index] *= db2lin(makeUpGain);
		}
	}

	if (recordOutput && nbChannels == recorder.header.NumOfChan)
		recorder.AppendData(output, nbFrames);
}
