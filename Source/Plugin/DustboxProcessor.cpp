/*
  ==============================================================================
  File: DustboxProcessor.cpp
  Responsibility: Implement the Dustbox AudioProcessor core flow, parameter
                  handling, and module coordination.
  Assumptions: DSP modules provide realtime-safe processBlock implementations.
  Notes: Finalises realtime routing with smoothing, equal-power mixing, and
         click-free bypass handling.
  ==============================================================================
*/

#include "DustboxProcessor.h"

#include "DustboxEditor.h"
#include "../Parameters/ParameterIDs.h"
#include "../Parameters/ParameterLayout.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

#include "../Dsp/utils/MathHelpers.h"

namespace dustbox
{

namespace
{
constexpr size_t maxProcessChannels = 16;
}

DustboxProcessor::DustboxProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "DustboxParameters", params::createParameterLayout())
{
}

void DustboxProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    const auto numChannels = getTotalNumInputChannels();

    tapeModule.prepare(sampleRate, samplesPerBlock, numChannels);
    dirtModule.prepare(sampleRate, samplesPerBlock, numChannels);
    pumpModule.prepare(sampleRate, samplesPerBlock, numChannels);

    dryBuffer.setSize(numChannels, samplesPerBlock);
    dryBuffer.clear();

    wetMixSmoother.reset(sampleRate, 30.0f);
    outputGainSmoother.reset(sampleRate, 30.0f);

    bypassSmoother.reset(sampleRate, 0.002);
    updateParameters();
    wetMixSmoother.setImmediate(cachedParameters.wetMix);
    outputGainSmoother.setImmediate(cachedParameters.outputGain);
    bypassSmoother.setCurrentAndTargetValue(cachedParameters.hardBypass ? 1.0f : 0.0f);

    tapeModule.reset();
    dirtModule.reset();
    pumpModule.reset();

    bypassTransitionActive = false;
}

void DustboxProcessor::releaseResources()
{
    dryBuffer.setSize(0, 0);
}

bool DustboxProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto mono = juce::AudioChannelSet::mono();
    auto stereo = juce::AudioChannelSet::stereo();
    return (layouts.getMainInputChannelSet() == mono || layouts.getMainInputChannelSet() == stereo)
           && (layouts.getMainOutputChannelSet() == mono || layouts.getMainOutputChannelSet() == stereo);
}

void DustboxProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    dsp::DenormalGuard guard;

    const auto numSamples = buffer.getNumSamples();
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear(channel, 0, numSamples);

    jassert(dryBuffer.getNumChannels() == totalNumInputChannels);
    jassert(numSamples <= dryBuffer.getNumSamples());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

    updateParameters();

    const float bypassTarget = cachedParameters.hardBypass ? 1.0f : 0.0f;
    if (bypassSmoother.getTargetValue() != bypassTarget)
    {
        bypassSmoother.setTargetValue(bypassTarget);
        bypassTransitionActive = true;
    }

    const bool bypassFullyEngaged = cachedParameters.hardBypass && ! bypassSmoother.isSmoothing()
                                    && juce::approximatelyEqual(bypassSmoother.getCurrentValue(), 1.0f);
    if (bypassFullyEngaged)
        return;

    hostTempo.updateFromPlayHead(getPlayHead());
    const auto samplesPerCycle = hostTempo.getSamplesPerCycle(currentSampleRate, cachedParameters.pumpParams.syncNoteIndex);
    pumpModule.setSync(samplesPerCycle, cachedParameters.pumpParams.phaseOffset);

    tapeModule.processBlock(buffer, numSamples);
    dirtModule.processBlock(buffer, numSamples);
    pumpModule.processBlock(buffer, numSamples);

    addNoisePostPump(buffer, numSamples);

    wetMixSmoother.setTarget(cachedParameters.wetMix);
    outputGainSmoother.setTarget(cachedParameters.outputGain);

    jassert(totalNumInputChannels <= static_cast<int>(maxProcessChannels));
    std::array<const float*, maxProcessChannels> dryPointers {};
    std::array<float*, maxProcessChannels> wetPointers {};

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        dryPointers[static_cast<size_t>(channel)] = dryBuffer.getReadPointer(channel);
        wetPointers[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto gains = dsp::equalPowerMixGains(wetMixSmoother.getNextValue());
        const auto outputGain = outputGainSmoother.getNextValue();

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            const auto channelIndex = static_cast<size_t>(channel);
            const float drySample = dryPointers[channelIndex][sample];
            const float wetSample = wetPointers[channelIndex][sample];
            wetPointers[channelIndex][sample] = (drySample * gains.dry + wetSample * gains.wet) * outputGain;
        }
    }

    addNoisePostMix(buffer, numSamples);
    applyBypassRamp(buffer, numSamples);
}

juce::AudioProcessorEditor* DustboxProcessor::createEditor()
{
    return new DustboxEditor(*this);
}

void DustboxProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = valueTreeState.copyState(); state.isValid())
    {
        juce::MemoryOutputStream stream(destData, false);
        state.writeToStream(stream);
    }
}

void DustboxProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ValueTree tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (tree.isValid())
        valueTreeState.replaceState(tree);
}

void DustboxProcessor::updateParameters()
{
    auto getFloat = [this](const char* id) {
        return valueTreeState.getRawParameterValue(id)->load();
    };
    auto getChoice = [this](const char* id) {
        return static_cast<int>(valueTreeState.getRawParameterValue(id)->load());
    };

    cachedParameters.tapeParams.wowDepth = getFloat(params::ids::tapeWowDepth);
    cachedParameters.tapeParams.wowRateHz = getFloat(params::ids::tapeWowRateHz);
    cachedParameters.tapeParams.flutterDepth = getFloat(params::ids::tapeFlutterDepth);
    cachedParameters.tapeParams.toneLowpassHz = getFloat(params::ids::tapeToneLowpassHz);
    cachedParameters.tapeParams.noiseLevelDb = getFloat(params::ids::tapeNoiseLevelDb);
    const auto noiseRouteIndex = juce::jlimit(0, 2, getChoice(params::ids::tapeNoiseRoute));
    cachedParameters.tapeParams.noiseRoute = static_cast<dsp::TapeModule::NoiseRoute>(noiseRouteIndex);

    tapeModule.setParameters(cachedParameters.tapeParams);

    cachedParameters.dirtParams.saturationAmount = getFloat(params::ids::dirtSaturationAmt);
    cachedParameters.dirtParams.bitDepth = getChoice(params::ids::dirtBitDepthBits);
    cachedParameters.dirtParams.sampleRateDiv = getChoice(params::ids::dirtSampleRateDiv);
    dirtModule.setParameters(cachedParameters.dirtParams);

    cachedParameters.pumpParams.amount = getFloat(params::ids::pumpAmount);
    cachedParameters.pumpParams.syncNoteIndex = juce::jlimit(0, 2, getChoice(params::ids::pumpSyncNote));
    cachedParameters.pumpParams.phaseOffset = getFloat(params::ids::pumpPhase);
    pumpModule.setParameters(cachedParameters.pumpParams);

    cachedParameters.wetMix = getFloat(params::ids::mixWet);
    cachedParameters.outputGain = juce::Decibels::decibelsToGain(getFloat(params::ids::outputGainDb));
    cachedParameters.hardBypass = valueTreeState.getRawParameterValue(params::ids::hardBypass)->load() > 0.5f;
}

void DustboxProcessor::applyBypassRamp(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (! bypassTransitionActive && ! cachedParameters.hardBypass)
        return;

    const auto numChannels = getTotalNumInputChannels();
    if (numChannels == 0)
        return;

    jassert(numChannels <= static_cast<int>(maxProcessChannels));
    std::array<const float*, maxProcessChannels> dryPointers {};
    std::array<float*, maxProcessChannels> wetPointers {};

    for (int channel = 0; channel < numChannels; ++channel)
    {
        dryPointers[static_cast<size_t>(channel)] = dryBuffer.getReadPointer(channel);
        wetPointers[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float bypassValue = bypassSmoother.getNextValue();
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto channelIndex = static_cast<size_t>(channel);
            const float drySample = dryPointers[channelIndex][sample];
            auto& wetSample = wetPointers[channelIndex][sample];
            wetSample = drySample * bypassValue + wetSample * (1.0f - bypassValue);
        }
    }

    if (! bypassSmoother.isSmoothing())
        bypassTransitionActive = false;
}

void DustboxProcessor::addNoisePostPump(juce::AudioBuffer<float>& wetBuffer, int numSamples)
{
    if (tapeModule.getNoiseRoute() != dsp::TapeModule::NoiseRoute::WetPostPump)
        return;

    const auto& noise = tapeModule.getNoiseBuffer();
    jassert(noise.getNumSamples() >= numSamples);
    const auto numChannels = wetBuffer.getNumChannels();
    for (int channel = 0; channel < numChannels; ++channel)
        wetBuffer.addFrom(channel, 0, noise, channel, 0, numSamples);
}

void DustboxProcessor::addNoisePostMix(juce::AudioBuffer<float>& mixBuffer, int numSamples)
{
    if (tapeModule.getNoiseRoute() != dsp::TapeModule::NoiseRoute::PostMix)
        return;

    const auto& noise = tapeModule.getNoiseBuffer();
    jassert(noise.getNumSamples() >= numSamples);
    const auto numChannels = mixBuffer.getNumChannels();
    for (int channel = 0; channel < numChannels; ++channel)
        mixBuffer.addFrom(channel, 0, noise, channel, 0, numSamples);
}
}

