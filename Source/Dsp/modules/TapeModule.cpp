/*
  ==============================================================================
  File: TapeModule.cpp
  Responsibility: Implement tape processing scaffolding and hiss routing logic.
  Assumptions: Actual modulation/tone algorithms will be filled in later.
  TODO: Add wow/flutter modulation, tone shaping, and refined noise modelling.
  ==============================================================================
*/

#include "TapeModule.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace dustbox::dsp
{
void TapeModule::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = samplesPerBlock;
    numChannelsPrepared = numChannels;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);

    wowDelay.prepare(spec);
    flutterDelay.prepare(spec);
    toneFilter.prepare(spec);
    toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    toneFilter.setCutoffFrequency(parameters.toneLowpassHz);

    noiseBuffer.setSize(numChannels, samplesPerBlock);
    noiseBuffer.clear();

    noiseGenerator.seed(0xC0FFEEu);
}

void TapeModule::reset()
{
    wowDelay.reset();
    flutterDelay.reset();
    toneFilter.reset();
}

void TapeModule::setParameters(const Parameters& newParams) noexcept
{
    parameters = newParams;
    toneFilter.setCutoffFrequency(parameters.toneLowpassHz);
}

void TapeModule::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    jassert(numSamples <= preparedBlockSize);
    const auto numChannels = buffer.getNumChannels();
    jassert(numChannels == numChannelsPrepared);

    const auto noiseGain = juce::Decibels::decibelsToGain(parameters.noiseLevelDb);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* noiseWrite = noiseBuffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
            noiseWrite[sample] = noiseGenerator.getNextSample() * noiseGain;
    }

    if (parameters.noiseRoute == NoiseRoute::WetPrePump)
    {
        for (int channel = 0; channel < numChannels; ++channel)
            buffer.addFrom(channel, 0, noiseBuffer, channel, 0, numSamples);
    }

    // TODO: Apply wow/flutter modulation and tone filtering once DSP is implemented.
}
} // namespace dustbox::dsp

