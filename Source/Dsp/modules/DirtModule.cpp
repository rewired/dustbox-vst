/*
  ==============================================================================
  File: DirtModule.cpp
  Responsibility: Implement scaffold for degradation processing (saturation,
                  quantisation, downsampling).
  Assumptions: Real DSP will replace the placeholders for tonal character.
  TODO: Replace placeholder saturation/quantiser with production algorithms.
  ==============================================================================
*/

#include "DirtModule.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <algorithm>

namespace dustbox::dsp
{
void DirtModule::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = samplesPerBlock;
    numChannelsPrepared = numChannels;

    downsampleCounters.assign(static_cast<size_t>(numChannels), 0);
    heldSamples.assign(static_cast<size_t>(numChannels), 0.0f);

    juce::ignoreUnused(sampleRate);
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

    const bool bypassQuantiser = parameters.bitDepth >= 24;
    const bool bypassDownsample = parameters.sampleRateDiv <= 1;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        auto counter = downsampleCounters[static_cast<size_t>(channel)];
        auto held = heldSamples[static_cast<size_t>(channel)];

        for (int i = 0; i < numSamples; ++i)
        {
            float sampleValue = data[i];

            if (parameters.saturationAmount > 0.0f)
            {
                const float drive = juce::jlimit(1.0f, 10.0f, 1.0f + parameters.saturationAmount * 4.0f);
                sampleValue = std::tanh(sampleValue * drive);
            }

            if (! bypassQuantiser)
            {
                const float steps = static_cast<float>(1 << juce::jlimit(1, 24, parameters.bitDepth));
                sampleValue = std::round(sampleValue * steps) / steps;
            }

            if (! bypassDownsample)
            {
                if (counter == 0)
                    held = sampleValue;

                sampleValue = held;
                counter = (counter + 1) % juce::jmax(1, parameters.sampleRateDiv);
            }

            data[i] = sampleValue;
        }

        downsampleCounters[static_cast<size_t>(channel)] = counter;
        heldSamples[static_cast<size_t>(channel)] = held;
    }
}
} // namespace dustbox::dsp

