/*
  ==============================================================================
  File: NoiseModule.h
  Responsibility: Generate broadband noise buffers according to the configured
                  level for routing within the Dustbox processor.
  Assumptions: prepare() sizes buffers and seeds generators; generate() is
               called once per block on the realtime thread.
  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

#include "../utils/NoiseGenerator.h"
#include "../utils/MathHelpers.h"

namespace dustbox::dsp
{
class NoiseModule
{
public:
    struct Parameters
    {
        float levelDb { -48.0f };
    };

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void setParameters(const Parameters& newParams) noexcept { parameters = newParams; }

    void generate(int numSamples) noexcept;

    const juce::AudioBuffer<float>& getNoiseBuffer() const noexcept { return noiseBuffer; }

private:
    static constexpr float noiseAudibleThreshold = 1.0e-6f;

    Parameters parameters {};

    juce::AudioBuffer<float> noiseBuffer;
    std::vector<NoiseGenerator> generators;

    int preparedBlockSize { 0 };
    int numChannelsPrepared { 0 };
};
} // namespace dustbox::dsp

