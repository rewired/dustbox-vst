/*
  ==============================================================================
  File: ReverbModule.cpp
  Responsibility: Implement a pre-delay plus algorithmic reverb stage with
                  internal wet-mix smoothing for the Dustbox wet path.
  Assumptions: prepare() is called before processBlock(); no allocations occur
               in processBlock().
  ==============================================================================
*/

#include "ReverbModule.h"

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

namespace dustbox::dsp
{
namespace
{
constexpr float minimumMix = 0.0f;
constexpr float maximumMix = 1.0f;
constexpr float mixSmoothingTimeSeconds = 0.05f;
constexpr float maximumPreDelaySeconds = 0.120f;

juce::dsp::Reverb::Parameters makeReverbParameters(const ReverbModule::Parameters& params)
{
    juce::dsp::Reverb::Parameters reverbParams;
    const auto roomSize = juce::jlimit(0.05f, 0.98f, params.decayTime / 8.0f);
    reverbParams.roomSize = roomSize;
    reverbParams.damping = juce::jlimit(0.0f, 1.0f, params.damping);
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    return reverbParams;
}
}

void ReverbModule::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = samplesPerBlock;
    numChannelsPrepared = numChannels;

    jassert(numChannels <= static_cast<int>(maxSupportedChannels));

    maxPreDelaySamples = static_cast<size_t>(std::ceil(maximumPreDelaySeconds * currentSampleRate)) + 1u;

    wetBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    wetBuffer.clear();

    mixSmoother.reset(sampleRate, mixSmoothingTimeSeconds);
    mixSmoother.setCurrentAndTargetValue(juce::jlimit(minimumMix, maximumMix, parameters.mix));

    for (int channel = 0; channel < static_cast<int>(maxSupportedChannels); ++channel)
    {
        reverbs[static_cast<size_t>(channel)].setSampleRate(sampleRate);
        reverbs[static_cast<size_t>(channel)].reset();

        auto& delayLine = preDelayLines[static_cast<size_t>(channel)];
        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1u };
        delayLine.prepare(spec);
        delayLine.setMaximumDelayInSamples(maxPreDelaySamples + static_cast<size_t>(samplesPerBlock));
        delayLine.reset();
    }

    updateReverbParameters();
}

void ReverbModule::reset()
{
    for (auto& reverb : reverbs)
        reverb.reset();

    for (auto& delayLine : preDelayLines)
        delayLine.reset();

    mixSmoother.setCurrentAndTargetValue(juce::jlimit(minimumMix, maximumMix, parameters.mix));
}

void ReverbModule::setParameters(const Parameters& newParams) noexcept
{
    parameters = newParams;
    updateReverbParameters();
}

void ReverbModule::processBlock(juce::AudioBuffer<float>& buffer, int numSamples, float mixTarget) noexcept
{
    jassert(numSamples <= preparedBlockSize);

    const auto numChannels = buffer.getNumChannels();
    jassert(numChannels <= numChannelsPrepared);
    jassert(numChannels <= static_cast<int>(maxSupportedChannels));

    const auto clampedMix = juce::jlimit(minimumMix, maximumMix, mixTarget);
    mixSmoother.setTargetValue(clampedMix);

    const auto desiredDelaySamples = juce::jlimit(0.0f,
                                                 static_cast<float>(maxPreDelaySamples),
                                                 parameters.preDelayMs * static_cast<float>(currentSampleRate) * 0.001f);
    const bool usePreDelay = desiredDelaySamples > 0.01f;

    std::array<const float*, maxSupportedChannels> dryPointers {};
    std::array<float*, maxSupportedChannels> writePointers {};
    std::array<float*, maxSupportedChannels> wetPointers {};

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto channelIndex = static_cast<size_t>(channel);
        dryPointers[channelIndex] = buffer.getReadPointer(channel);
        writePointers[channelIndex] = buffer.getWritePointer(channel);
        wetPointers[channelIndex] = wetBuffer.getWritePointer(channel);

        auto& delayLine = preDelayLines[channelIndex];
        if (usePreDelay)
        {
            delayLine.setDelay(desiredDelaySamples);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                const auto inputSample = dryPointers[channelIndex][sample];
                const auto delayedSample = delayLine.popSample(0);
                delayLine.pushSample(0, inputSample);
                wetPointers[channelIndex][sample] = delayedSample;
            }
        }
        else
        {
            juce::FloatVectorOperations::copy(wetPointers[channelIndex], dryPointers[channelIndex], numSamples);
            delayLine.reset();
        }

        reverbs[channelIndex].processMono(wetPointers[channelIndex], numSamples);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto wetLevel = mixSmoother.getNextValue();
        const auto dryLevel = 1.0f - wetLevel;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto channelIndex = static_cast<size_t>(channel);
            const auto drySample = dryPointers[channelIndex][sample];
            const auto wetSample = wetPointers[channelIndex][sample];
            writePointers[channelIndex][sample] = drySample * dryLevel + wetSample * wetLevel;
        }
    }
}

void ReverbModule::updateReverbParameters()
{
    const auto juceParams = makeReverbParameters(parameters);
    for (auto& reverb : reverbs)
        reverb.setParameters(juceParams);
}
} // namespace dustbox::dsp

