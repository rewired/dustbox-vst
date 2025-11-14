/*
  ==============================================================================
  File: ReverbModule.h
  Responsibility: Define a lightweight stereo reverb stage with optional
                  pre-delay for Dustbox's wet processing path.
  Assumptions: Parameters are updated atomically from the processor; module
               methods are called from the audio thread after prepare().
  Notes: Uses per-channel JUCE Reverb instances and keeps scratch buffers
         preallocated to remain realtime safe.
  ==============================================================================
*/

#pragma once

#include <array>

#include <juce_dsp/juce_dsp.h>

namespace dustbox::dsp
{
class ReverbModule
{
public:
    struct Parameters
    {
        float preDelayMs { 20.0f };
        float decayTime { 1.8f };
        float damping { 0.35f };
        float mix { 0.25f };
    };

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void setParameters(const Parameters& newParams) noexcept;
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples, float mixTarget) noexcept;

private:
    static constexpr size_t maxSupportedChannels = 16;

    Parameters parameters {};

    double currentSampleRate { 44100.0 };
    int preparedBlockSize { 0 };
    int numChannelsPrepared { 0 };
    size_t maxPreDelaySamples { 0 };

    std::array<juce::dsp::Reverb, maxSupportedChannels> reverbs {};
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, maxSupportedChannels>
        preDelayLines {};

    juce::AudioBuffer<float> wetBuffer;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;

    void updateReverbParameters();
};
} // namespace dustbox::dsp

