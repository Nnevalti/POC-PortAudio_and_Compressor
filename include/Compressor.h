#pragma once

class Compressor
{
public:
	Compressor();
	~Compressor();

	float sampleRate;

	float attack;
	float release;
	float threshold;
	float ratio;
	float makeUpGain;
	float knee;
	float envelope;

	static inline float db2lin(float db);
	static inline float lin2db(float lin);

	// void process(const float *input, float *output, int numSamples, int numChannels);
	float *Compress(const float* input, const unsigned int nbFrames, const unsigned int nbChannels);
};