#pragma once

#include "WavRecorder.h"

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

	WavRecorder recorder;

	static inline float db2lin(float db);
	static inline float lin2db(float lin);
	static inline float kneeWidth2lin(float knee);

	// void process(const float *input, float *output, int numSamples, int numChannels);
	float *Compress(const float* input, const unsigned int nbFrames, const unsigned int nbChannels);
};