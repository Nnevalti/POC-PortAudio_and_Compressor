#include "Compressor.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace {

bool expectNear(const char* name, float actual, float expected, float tolerance = 1e-5f)
{
    if (std::fabs(actual - expected) <= tolerance)
        return true;

    std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
    return false;
}

bool expectFinite(const char* name, float value)
{
    if (std::isfinite(value))
        return true;

    std::cerr << name << ": expected a finite value\n";
    return false;
}

std::unique_ptr<float[]> compress(Compressor& compressor, const float* input, unsigned int frames)
{
    return std::unique_ptr<float[]>(compressor.Compress(input, frames, 2));
}

float fromDb(float decibels)
{
    return std::pow(10.0f, decibels / 20.0f);
}

float toDb(float amplitude)
{
    return 20.0f * std::log10(amplitude);
}

} // namespace

int main()
{
    bool passed = true;

    Compressor compressor(false);
    compressor.threshold = -20.0f;
    compressor.ratio = 4.0f;
    compressor.knee = 0.0f;
    compressor.makeUpGain = 0.0f;

    const float hardKneeInput[] = {0.05f, -0.05f, 0.10f, -fromDb(-10.0f)};
    const auto hardKneeOutput = compress(compressor, hardKneeInput, 2);
    passed &= expectNear("below threshold", hardKneeOutput[0], 0.05f);
    passed &= expectNear("negative below threshold", hardKneeOutput[1], -0.05f);
    passed &= expectNear("at threshold", hardKneeOutput[2], 0.10f);
    passed &= expectFinite("at threshold", hardKneeOutput[2]);
    passed &= expectNear("above threshold", hardKneeOutput[3], -fromDb(-17.5f));

    compressor.makeUpGain = 6.0f;
    const float makeUpInput[] = {0.10f, -0.10f};
    const auto makeUpOutput = compress(compressor, makeUpInput, 1);
    const float makeUpMultiplier = std::pow(10.0f, 6.0f / 20.0f);
    passed &= expectNear("make-up gain left", makeUpOutput[0], 0.10f * makeUpMultiplier);
    passed &= expectNear("make-up gain right", makeUpOutput[1], -0.10f * makeUpMultiplier);

    compressor.makeUpGain = 0.0f;
    compressor.knee = 10.0f;
    const float softKneeInput[] = {0.0f, fromDb(-20.0f), fromDb(-15.0f), fromDb(-25.0f)};
    const auto softKneeOutput = compress(compressor, softKneeInput, 2);
    passed &= expectNear("silence", softKneeOutput[0], 0.0f);
    passed &= expectNear("soft knee centre", toDb(softKneeOutput[1]), -20.9375f);
    passed &= expectNear("soft knee upper edge", toDb(softKneeOutput[2]), -18.75f);
    passed &= expectNear("soft knee lower edge", toDb(softKneeOutput[3]), -25.0f);

    // The live path writes directly into PortAudio's buffer.
    float directOutput[4] = {};
    compressor.process(softKneeInput, directOutput, 2, 2);
    for (unsigned int i = 0; i < 4; ++i)
        passed &= expectNear("caller-owned buffer", directOutput[i], softKneeOutput[i]);

    float inPlace[] = {fromDb(-20.0f), -fromDb(-20.0f)};
    compressor.process(inPlace, inPlace, 1, 2);
    passed &= expectNear("in-place positive", inPlace[0], softKneeOutput[1]);
    passed &= expectNear("in-place negative", inPlace[1], -softKneeOutput[1]);

    compressor.process(nullptr, directOutput, 2, 2);
    for (float sample : directOutput)
        passed &= expectNear("missing input produces silence", sample, 0.0f);

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
