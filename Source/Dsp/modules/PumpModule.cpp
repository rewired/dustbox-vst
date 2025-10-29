/*
  ==============================================================================
  File: PumpModule.cpp
  Responsibility: Implement tempo-synchronised gain modulation with a fast
                  decay and eased release envelope.
  Assumptions: setSync() is called once per block. No allocations occur in
               processBlock().
  ==============================================================================
*/

#include "PumpModule.h"

#include <array>
#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

namespace dustbox::dsp
{
namespace
{
constexpr float decayPortion = 0.28f;
constexpr float minimumGain = 0.05f;
}

void PumpModule::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = samplesPerBlock;
    numChannelsPrepared = numChannels;

    jassert(numChannels <= static_cast<int>(maxSupportedChannels));
}

void PumpModule::reset()
{
    phase = 0.0;
}

void PumpModule::setParameters(const Parameters& newParams) noexcept
{
    parameters = newParams;
}

void PumpModule::setSync(double newSamplesPerCycle, float phaseOffsetNormalised) noexcept
{
    samplesPerCycle = juce::jmax(1.0, newSamplesPerCycle);
    phaseIncrement = 1.0 / samplesPerCycle;
    phaseOffset = juce::jlimit(0.0f, 1.0f, phaseOffsetNormalised);
}

void PumpModule::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    jassert(numSamples <= preparedBlockSize);
    const auto numChannels = buffer.getNumChannels();
    jassert(numChannels == numChannelsPrepared);
    jassert(numChannels <= static_cast<int>(maxSupportedChannels));

    std::array<float*, maxSupportedChannels> channelPointers {};
    for (int channel = 0; channel < numChannels; ++channel)
        channelPointers[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);

    auto localPhase = phase;
    const auto increment = phaseIncrement;
    const auto offset = phaseOffset;
    const auto amount = juce::jlimit(0.0f, 1.0f, parameters.amount);
    const auto depth = amount * amount;
    const auto minGain = juce::jlimit(minimumGain, 1.0f, 1.0f - depth * 0.9f);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto phaseWithOffset = localPhase + static_cast<double>(offset);
        if (phaseWithOffset >= 1.0)
            phaseWithOffset -= 1.0;
        const auto phase01 = static_cast<float>(phaseWithOffset);

        float envelope = 1.0f;
        if (amount > 0.0001f)
        {
            if (phase01 < decayPortion)
            {
                const auto t = juce::jlimit(0.0f, 1.0f, phase01 / decayPortion);
                auto fall = 1.0f - t;
                fall *= fall;
                envelope = minGain + (1.0f - minGain) * fall;
            }
            else
            {
                const auto t = juce::jlimit(0.0f, 1.0f, (phase01 - decayPortion) / (1.0f - decayPortion));
                const auto smooth = t * t * (3.0f - 2.0f * t);
                envelope = minGain + (1.0f - minGain) * smooth;
            }
        }

        for (int channel = 0; channel < numChannels; ++channel)
            channelPointers[static_cast<size_t>(channel)][sample] *= envelope;

        localPhase += increment;
        if (localPhase >= 1.0)
            localPhase -= 1.0;
    }

    phase = localPhase;
}
} // namespace dustbox::dsp

