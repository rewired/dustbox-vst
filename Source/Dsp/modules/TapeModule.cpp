/*
  ==============================================================================
  File: TapeModule.cpp
  Responsibility: Implement tape processing with modulated delay and tone
                  roll-off for the Dustbox signal chain.
  Assumptions: Parameters are refreshed from the processor prior to processing
               each block. prepare() sizes all buffers; no allocations occur in
               processBlock().
  ==============================================================================
*/

#include "TapeModule.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

namespace dustbox::dsp
{
namespace
{
constexpr size_t maxSupportedChannels = 16; // Hard cap to keep stack allocations bounded.
constexpr float minDelaySamples = 1.0f;
constexpr float toneUpdateThreshold = 1.0e-3f;
} // namespace

void TapeModule::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    jassert(sampleRate > 0.0);
    jassert(numChannels <= static_cast<int>(maxSupportedChannels));
    currentSampleRate = sampleRate;
    preparedBlockSize = samplesPerBlock;
    numChannelsPrepared = numChannels;

    baseDelaySamples = static_cast<float>(sampleRate * (baseDelayMs * 0.001));
    wowDepthSamplesRange = static_cast<float>(sampleRate * (maxWowDepthMs * 0.001));
    flutterDepthSamplesRange = static_cast<float>(sampleRate * (maxFlutterDepthMs * 0.001));

    delayBufferSize = static_cast<int>(std::ceil(sampleRate * (maxDelayMs * 0.001f))) + 4;
    delayBuffer.setSize(numChannels, delayBufferSize, false, false, true);
    delayBuffer.clear();

    writePositions.assign(static_cast<size_t>(numChannels), 0);
    toneStates.assign(static_cast<size_t>(numChannels), 0.0f);

    toneCutoff.reset(sampleRate, 0.03f);
    toneCutoff.setCurrentAndTargetValue(parameters.toneLowpassHz);
    lastToneCutoffHz = parameters.toneLowpassHz;
    toneCoefficient = computeToneCoefficient(lastToneCutoffHz);

    wowPhase = 0.0f;
    flutterPhase = 0.0f;
}

void TapeModule::reset()
{
    delayBuffer.clear();
    std::fill(writePositions.begin(), writePositions.end(), 0);
    std::fill(toneStates.begin(), toneStates.end(), 0.0f);

    toneCutoff.setCurrentAndTargetValue(parameters.toneLowpassHz);
    lastToneCutoffHz = parameters.toneLowpassHz;
    toneCoefficient = computeToneCoefficient(lastToneCutoffHz);

    wowPhase = 0.0f;
    flutterPhase = 0.0f;
}

void TapeModule::setParameters(const Parameters& newParams) noexcept
{
    parameters = newParams;
    toneCutoff.setTargetValue(parameters.toneLowpassHz);
}

float TapeModule::computeToneCoefficient(float cutoffHz) const noexcept
{
    const auto clampedCutoff = juce::jlimit(20.0f, static_cast<float>(0.5 * currentSampleRate - 10.0), cutoffHz);
    const auto omega = juce::MathConstants<float>::twoPi * clampedCutoff / static_cast<float>(currentSampleRate);
    const auto expTerm = std::exp(-omega);
    return 1.0f - expTerm;
}

void TapeModule::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    jassert(numSamples <= preparedBlockSize);
    const auto numChannels = buffer.getNumChannels();
    jassert(numChannels == numChannelsPrepared);
    jassert(numChannels <= static_cast<int>(maxSupportedChannels));

    const auto wowRate = juce::jlimit(0.1f, 5.0f, parameters.wowRateHz);
    const auto flutterRate = juce::jlimit(5.0f, 9.5f, 5.0f + parameters.wowRateHz * 0.9f);

    const auto wowDepthSamples = wowDepthSamplesRange * juce::jlimit(0.0f, 1.0f, parameters.wowDepth);
    const auto flutterDepthSamples = flutterDepthSamplesRange * juce::jlimit(0.0f, 1.0f, parameters.flutterDepth);

    const auto wowIncrement = wowRate > 0.0f
                                  ? juce::MathConstants<float>::twoPi * wowRate / static_cast<float>(currentSampleRate)
                                  : 0.0f;
    const auto flutterIncrement = flutterRate > 0.0f
                                       ? juce::MathConstants<float>::twoPi * flutterRate / static_cast<float>(currentSampleRate)
                                       : 0.0f;

    const auto maxDelaySamplesFloat = static_cast<float>(delayBufferSize - 2);
    std::array<float*, maxSupportedChannels> bufferPointers {};
    std::array<float*, maxSupportedChannels> delayPointers {};

    for (int channel = 0; channel < numChannels; ++channel)
    {
        bufferPointers[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);
        delayPointers[static_cast<size_t>(channel)] = delayBuffer.getWritePointer(channel);
    }

    auto localWowPhase = wowPhase;
    auto localFlutterPhase = flutterPhase;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto cutoff = toneCutoff.getNextValue();
        if (std::abs(cutoff - lastToneCutoffHz) > toneUpdateThreshold)
        {
            toneCoefficient = computeToneCoefficient(cutoff);
            lastToneCutoffHz = cutoff;
        }

        const auto wowMod = wowDepthSamples * std::sin(localWowPhase);
        const auto flutterMod = flutterDepthSamples * std::sin(localFlutterPhase);
        auto delaySamples = baseDelaySamples + wowMod + flutterMod;
        delaySamples = juce::jlimit(minDelaySamples, maxDelaySamplesFloat, delaySamples);

        localWowPhase += wowIncrement;
        if (localWowPhase >= juce::MathConstants<float>::twoPi)
            localWowPhase -= juce::MathConstants<float>::twoPi;

        localFlutterPhase += flutterIncrement;
        if (localFlutterPhase >= juce::MathConstants<float>::twoPi)
            localFlutterPhase -= juce::MathConstants<float>::twoPi;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto channelIndex = static_cast<size_t>(channel);
            auto* audio = bufferPointers[channelIndex];
            auto* delay = delayPointers[channelIndex];

            auto writeIndex = writePositions[channelIndex];
            const auto inputSample = audio[sample];
            delay[writeIndex] = inputSample;

            auto readPosition = static_cast<float>(writeIndex) - delaySamples;
            if (readPosition < 0.0f)
                readPosition += static_cast<float>(delayBufferSize);

            const auto index0 = static_cast<int>(readPosition);
            auto index1 = index0 + 1;
            if (index1 >= delayBufferSize)
                index1 -= delayBufferSize;

            const auto frac = readPosition - static_cast<float>(index0);
            const auto delayed0 = delay[index0];
            const auto delayed1 = delay[index1];
            const auto delayedSample = delayed0 + (delayed1 - delayed0) * frac;

            auto& toneState = toneStates[channelIndex];
            toneState += toneCoefficient * (delayedSample - toneState);

            audio[sample] = toneState;

            ++writeIndex;
            if (writeIndex >= delayBufferSize)
                writeIndex = 0;
            writePositions[channelIndex] = writeIndex;
        }
    }

    wowPhase = localWowPhase;
    flutterPhase = localFlutterPhase;
}
} // namespace dustbox::dsp

