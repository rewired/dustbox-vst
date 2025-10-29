/*
  ==============================================================================
  File: TapeModule.h
  Responsibility: Encapsulate tape coloration processing (wow/flutter, tone,
                  hiss routing) for the Dustbox signal chain.
  Assumptions: prepare() is called before processBlock and parameters are
               updated from the owning processor.
  Notes: DSP implementation keeps allocations outside the realtime path and
         remains zero-latency.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

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
    static constexpr float baseDelayMs = 12.0f;
    static constexpr float maxWowDepthMs = 6.0f;
    static constexpr float maxFlutterDepthMs = 1.2f;
    static constexpr float maxDelayMs = baseDelayMs + maxWowDepthMs + maxFlutterDepthMs + 4.0f;

    float computeToneCoefficient(float cutoffHz) const noexcept;

    Parameters parameters {};

    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> noiseBuffer;

    std::vector<int> writePositions;
    std::vector<float> toneStates;
    std::vector<NoiseGenerator> noiseGenerators;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneCutoff;

    double currentSampleRate { 44100.0 };
    float toneCoefficient { 0.0f };
    float lastToneCutoffHz { 0.0f };

    float baseDelaySamples { 0.0f };
    float wowDepthSamplesRange { 0.0f };
    float flutterDepthSamplesRange { 0.0f };

    int delayBufferSize { 0 };
    int preparedBlockSize { 0 };
    int numChannelsPrepared { 0 };

    float wowPhase { 0.0f };
    float flutterPhase { 0.0f };
};
} // namespace dustbox::dsp

