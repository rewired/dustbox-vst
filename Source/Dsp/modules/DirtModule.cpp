/*
  ==============================================================================
  File: DirtModule.cpp
  Responsibility: Implement lo-fi degradation (saturation, bit quantisation,
                  downsampling) for the Dustbox signal chain.
  Assumptions: Parameters are refreshed each block. No allocations happen in the
               realtime path and all processing is in-place.
  ==============================================================================
*/

#include "DirtModule.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../utils/MathHelpers.h"

namespace dustbox::dsp
{
namespace
{
constexpr size_t maxSupportedChannels = 16;
constexpr float saturationFloor = 1.0e-4f;
}

void DirtModule::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = samplesPerBlock;
    numChannelsPrepared = numChannels;

    jassert(numChannels <= static_cast<int>(maxSupportedChannels));

    downsampleCounters.assign(static_cast<size_t>(numChannels), 0);
    heldSamples.assign(static_cast<size_t>(numChannels), 0.0f);
}

void DirtModule::reset()
{
    std::fill(downsampleCounters.begin(), downsampleCounters.end(), 0);
    std::fill(heldSamples.begin(), heldSamples.end(), 0.0f);
}

void DirtModule::setParameters(const Parameters& newParams) noexcept
{
    parameters = newParams;
}

void DirtModule::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    jassert(numSamples <= preparedBlockSize);
    const auto numChannels = buffer.getNumChannels();
    jassert(numChannels == numChannelsPrepared);
    jassert(numChannels <= static_cast<int>(maxSupportedChannels));

    const auto saturationAmount = juce::jlimit(0.0f, 1.0f, parameters.saturationAmount);
    const auto applySaturation = saturationAmount > saturationFloor;
    const auto drive = applySaturation ? (1.0f + 10.0f * saturationAmount * saturationAmount) : 1.0f;
    const auto inverseDrive = 1.0f / drive;

    const auto bitDepth = juce::jlimit(4, 24, parameters.bitDepth);
    const auto bypassQuantiser = bitDepth >= 24;
    const auto levelCount = static_cast<float>(std::ldexp(1.0, bitDepth) - 1.0);
    const auto step = 2.0f / levelCount;

    const auto divider = juce::jmax(1, parameters.sampleRateDiv);
    const auto bypassDownsample = divider <= 1;

    std::array<float*, maxSupportedChannels> channelPointers {};

    for (int channel = 0; channel < numChannels; ++channel)
        channelPointers[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto channelIndex = static_cast<size_t>(channel);
        auto* data = channelPointers[channelIndex];
        auto counter = downsampleCounters[channelIndex];
        auto held = heldSamples[channelIndex];

        for (int sample = 0; sample < numSamples; ++sample)
        {
            auto value = data[sample];

            if (applySaturation)
            {
                const auto driven = softClip(value * drive);
                value = driven * inverseDrive;
            }

            if (! bypassQuantiser)
            {
                const auto clamped = juce::jlimit(-1.0f, 1.0f, value);
                value = std::round(clamped / step) * step;
            }

            if (! bypassDownsample)
            {
                if (counter <= 0)
                {
                    held = value;
                    counter = divider;
                }

                value = held;
                --counter;
            }

            data[sample] = value;
        }

        downsampleCounters[channelIndex] = counter;
        heldSamples[channelIndex] = held;
    }
}
} // namespace dustbox::dsp

