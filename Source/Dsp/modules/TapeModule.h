/*
  ==============================================================================
  File: TapeModule.h
  Responsibility: Encapsulate tape coloration processing (wow/flutter, tone,
                  hiss routing) for the Dustbox signal chain.
  Assumptions: prepare() is called before processBlock and parameters are
               updated from the owning processor.
  TODO: Implement actual modulated delay, tone shaping, and hiss modelling.
  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>

#include "../utils/NoiseGenerator.h"

namespace dustbox::dsp
{
class TapeModule
{
public:
    enum class NoiseRoute
    {
        WetPrePump = 0,
        WetPostPump,
        PostMix
    };

    struct Parameters
    {
        float wowDepth { 0.15f };
        float wowRateHz { 0.60f };
        float flutterDepth { 0.08f };
        float toneLowpassHz { 11000.0f };
        float noiseLevelDb { -48.0f };
        NoiseRoute noiseRoute { NoiseRoute::WetPrePump };
    };

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void setParameters(const Parameters& newParams) noexcept;

    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

    const juce::AudioBuffer<float>& getNoiseBuffer() const noexcept { return noiseBuffer; }
    NoiseRoute getNoiseRoute() const noexcept { return parameters.noiseRoute; }

private:
    Parameters parameters {};
    juce::dsp::DelayLine<float> wowDelay { 2048 };
    juce::dsp::DelayLine<float> flutterDelay { 1024 };
    juce::dsp::StateVariableTPTFilter<float> toneFilter;
    NoiseGenerator noiseGenerator {};
    juce::AudioBuffer<float> noiseBuffer;

    double currentSampleRate { 44100.0 };
    int preparedBlockSize { 0 };
    int numChannelsPrepared { 0 };
};
} // namespace dustbox::dsp

