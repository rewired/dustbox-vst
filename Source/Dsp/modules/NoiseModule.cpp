/*
  ==============================================================================
  File: NoiseModule.cpp
  Responsibility: Implement noise generation for routing within Dustbox.
  Assumptions: prepare() is invoked before processing; generate() executes on
               the realtime thread without allocations.
  ==============================================================================
*/

#include "NoiseModule.h"

#include <array>

namespace dustbox::dsp
{
namespace
{
constexpr size_t maxSupportedChannels = 16;
constexpr uint32_t baseSeed = 0xC0FFEEu;
constexpr uint32_t seedStride = 131u;
} // namespace

void NoiseModule::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused(sampleRate);

    jassert(numChannels <= static_cast<int>(maxSupportedChannels));
    preparedBlockSize = samplesPerBlock;
    numChannelsPrepared = numChannels;

    noiseBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    noiseBuffer.clear();

    generators.resize(static_cast<size_t>(numChannels));
    for (size_t i = 0; i < generators.size(); ++i)
        generators[i].seed(baseSeed + static_cast<uint32_t>(i) * seedStride);
}

void NoiseModule::reset()
{
    noiseBuffer.clear();
    for (size_t i = 0; i < generators.size(); ++i)
        generators[i].seed(baseSeed + static_cast<uint32_t>(i) * seedStride);
}

void NoiseModule::generate(int numSamples) noexcept
{
    jassert(numSamples <= preparedBlockSize);
    const auto numChannels = noiseBuffer.getNumChannels();
    jassert(numChannels == numChannelsPrepared);

    const auto gain = dbToGain(parameters.levelDb);
    const bool noiseActive = gain > noiseAudibleThreshold;

    if (! noiseActive)
    {
        for (int channel = 0; channel < numChannels; ++channel)
            noiseBuffer.clear(channel, 0, numSamples);
        return;
    }

    std::array<float*, maxSupportedChannels> channelPointers {};
    for (int channel = 0; channel < numChannels; ++channel)
        channelPointers[static_cast<size_t>(channel)] = noiseBuffer.getWritePointer(channel);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto index = static_cast<size_t>(channel);
            channelPointers[index][sample] = generators[index].getNextSample() * gain;
        }
    }
}
} // namespace dustbox::dsp

