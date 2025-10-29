/*
  ==============================================================================
  File: PumpModule.cpp
  Responsibility: Implement tempo-synchronised gain envelope scaffolding.
  Assumptions: Host sync data is refreshed each block via setSync().
  TODO: Tune envelope curve constants for musical sidechain feel.
  ==============================================================================
*/

#include "PumpModule.h"

#include <cmath>
#include <array>
#include <juce_audio_basics/juce_audio_basics.h>

namespace dustbox::dsp
{
void PumpModule::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = samplesPerBlock;
    numChannelsPrepared = numChannels;

    juce::ignoreUnused(sampleRate, samplesPerBlock, numChannels);
}

void PumpModule::reset()
{
    phase = 0.0;
}

void PumpModule::setParameters(const Parameters& newParams) noexcept
{
    parameters = newParams;
}

void PumpModule::setSync(double newSamplesPerCycle, double phaseOffsetSamples) noexcept
{
    samplesPerCycle = newSamplesPerCycle > 1.0 ? newSamplesPerCycle : 1.0;
    phaseOffset = phaseOffsetSamples;
}

void PumpModule::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    jassert(numSamples <= preparedBlockSize);
    const auto numChannels = buffer.getNumChannels();
    jassert(numChannels == numChannelsPrepared);

    const double cycle = samplesPerCycle > 0.0 ? samplesPerCycle : static_cast<double>(numSamples);
    const double invCycle = 1.0 / cycle;

    double localPhase = std::fmod(phase + phaseOffset, cycle);

    constexpr size_t maxPointers = 8;
    std::array<float*, maxPointers> channelPointers {};
    jassert(numChannels <= static_cast<int>(maxPointers));
    for (int channel = 0; channel < numChannels; ++channel)
        channelPointers[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const double phase01 = std::fmod(localPhase, cycle) * invCycle;
        const float pumpAmount = juce::jlimit(0.0f, 1.0f, parameters.amount);
        const float decay = std::exp(-6.0f * static_cast<float>(phase01));
        const float release = static_cast<float>(phase01);
        const float easedRelease = release * release;
        const float envelope = 1.0f - pumpAmount * (1.0f - decay * (1.0f - easedRelease));

        for (int channel = 0; channel < numChannels; ++channel)
            channelPointers[static_cast<size_t>(channel)][sample] *= envelope;

        localPhase += 1.0;
        if (localPhase >= cycle)
            localPhase -= cycle;
    }

    phase = localPhase;
}
} // namespace dustbox::dsp

