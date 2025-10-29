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
#include <utility>

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
    initialiseFactoryPresets();
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

    publishMeterReadings(dryBuffer, inputMeterValues, totalNumInputChannels, numSamples);

    const float bypassTarget = cachedParameters.hardBypass ? 1.0f : 0.0f;
    if (bypassSmoother.getTargetValue() != bypassTarget)
    {
        bypassSmoother.setTargetValue(bypassTarget);
        bypassTransitionActive = true;
    }

    hostTempo.updateFromPlayHead(getPlayHead());
    const auto syncNoteIndex = cachedParameters.pumpParams.syncNoteIndex;

    const bool bypassFullyEngaged = cachedParameters.hardBypass && ! bypassSmoother.isSmoothing()
                                    && juce::approximatelyEqual(bypassSmoother.getCurrentValue(), 1.0f);
    if (bypassFullyEngaged)
    {
        publishMeterReadings(dryBuffer, outputMeterValues, totalNumInputChannels, numSamples);
        hostTempo.advanceFallbackPhase(numSamples, currentSampleRate, syncNoteIndex);
        return;
    }

    const auto samplesPerCycle = hostTempo.getSamplesPerCycle(currentSampleRate, syncNoteIndex);
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

    publishMeterReadings(buffer, outputMeterValues, totalNumOutputChannels, numSamples);
    hostTempo.advanceFallbackPhase(numSamples, currentSampleRate, syncNoteIndex);
}

juce::AudioProcessorEditor* DustboxProcessor::createEditor()
{
    return new DustboxEditor(*this);
}

int DustboxProcessor::getNumPrograms()
{
    return static_cast<int>(factoryPresets.size());
}

int DustboxProcessor::getCurrentProgram()
{
    if (factoryPresets.empty())
        return 0;

    return juce::jlimit(0, getNumPrograms() - 1, currentProgramIndex);
}

void DustboxProcessor::setCurrentProgram(int index)
{
    if (factoryPresets.empty())
        return;

    const int clamped = juce::jlimit(0, getNumPrograms() - 1, index);
    if (clamped == currentProgramIndex)
        return;

    const auto& preset = factoryPresets[static_cast<size_t>(clamped)];
    valueTreeState.replaceState(preset.state.createCopy());
    currentProgramIndex = clamped;
    updateHostDisplay();
}

const juce::String DustboxProcessor::getProgramName(int index)
{
    if (index < 0 || index >= getNumPrograms() || factoryPresets.empty())
        return {};

    return factoryPresets[static_cast<size_t>(index)].name;
}

void DustboxProcessor::changeProgramName(int index, const juce::String& newName)
{
    if (index < 0 || index >= getNumPrograms() || factoryPresets.empty())
        return;

    factoryPresets[static_cast<size_t>(index)].name = newName;
}

void DustboxProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // JUCE 7 exposed copyState(); JUCE 8 made it non-const, so grab the ValueTree directly.
    auto state = valueTreeState.state;
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void DustboxProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        auto restoredState = juce::ValueTree::fromXml(*xml);

        if (! restoredState.isValid())
            return;

        // JUCE 8 prefers replaceState with an explicit ValueTree instead of copyState round-trips.
        valueTreeState.replaceState(restoredState);

        if (! factoryPresets.empty())
        {
            const auto match = findPresetIndexMatchingState(restoredState);
            if (match >= 0)
                currentProgramIndex = match;
        }
    }
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

void DustboxProcessor::initialiseFactoryPresets()
{
    using namespace params::ids;

    factoryPresets.clear();
    factoryPresets.reserve(5);

    auto addPreset = [this](juce::String name, const std::function<void(juce::ValueTree&)>& mutator)
    {
        factoryPresets.push_back({ std::move(name), createPresetState(mutator) });
    };

    auto setParam = [](juce::ValueTree& state, const char* paramID, auto value)
    {
        state.setProperty(juce::Identifier(paramID), value, nullptr);
    };

    addPreset("Subtle Tape Glue", [setParam](juce::ValueTree& state)
    {
        setParam(state, tapeWowDepth, 0.10f);
        setParam(state, tapeWowRateHz, 0.50f);
        setParam(state, tapeFlutterDepth, 0.05f);
        setParam(state, tapeToneLowpassHz, 14000.0f);
        setParam(state, tapeNoiseLevelDb, -48.0f);
        setParam(state, tapeNoiseRoute, 0);
        setParam(state, dirtSaturationAmt, 0.0f);
        setParam(state, dirtBitDepthBits, 24);
        setParam(state, dirtSampleRateDiv, 1);
        setParam(state, pumpAmount, 0.0f);
        setParam(state, pumpSyncNote, 1);
        setParam(state, pumpPhase, 0.0f);
        setParam(state, mixWet, 0.25f);
        setParam(state, outputGainDb, 0.0f);
        setParam(state, hardBypass, false);
    });

    addPreset("Lo-Fi Sampler", [setParam](juce::ValueTree& state)
    {
        setParam(state, tapeWowDepth, 0.08f);
        setParam(state, tapeWowRateHz, 0.80f);
        setParam(state, tapeFlutterDepth, 0.04f);
        setParam(state, tapeToneLowpassHz, 9500.0f);
        setParam(state, dirtSaturationAmt, 0.5f);
        setParam(state, dirtBitDepthBits, 8);
        setParam(state, dirtSampleRateDiv, 8);
        setParam(state, pumpAmount, 0.0f);
        setParam(state, pumpSyncNote, 1);
        setParam(state, pumpPhase, 0.0f);
        setParam(state, mixWet, 0.6f);
        setParam(state, outputGainDb, -1.0f);
        setParam(state, hardBypass, false);
    });

    addPreset("Side-Chain Duck", [setParam](juce::ValueTree& state)
    {
        setParam(state, tapeWowDepth, 0.12f);
        setParam(state, tapeWowRateHz, 0.60f);
        setParam(state, tapeFlutterDepth, 0.06f);
        setParam(state, dirtSaturationAmt, 0.2f);
        setParam(state, dirtBitDepthBits, 24);
        setParam(state, dirtSampleRateDiv, 1);
        setParam(state, pumpAmount, 0.6f);
        setParam(state, pumpSyncNote, 1);
        setParam(state, pumpPhase, 0.0f);
        setParam(state, mixWet, 1.0f);
        setParam(state, outputGainDb, 0.0f);
        setParam(state, hardBypass, false);
    });

    addPreset("Noisy VHS", [setParam](juce::ValueTree& state)
    {
        setParam(state, tapeWowDepth, 0.25f);
        setParam(state, tapeWowRateHz, 0.50f);
        setParam(state, tapeFlutterDepth, 0.18f);
        setParam(state, tapeToneLowpassHz, 7000.0f);
        setParam(state, tapeNoiseLevelDb, -36.0f);
        setParam(state, tapeNoiseRoute, 2);
        setParam(state, dirtSaturationAmt, 0.25f);
        setParam(state, dirtBitDepthBits, 16);
        setParam(state, dirtSampleRateDiv, 2);
        setParam(state, pumpAmount, 0.2f);
        setParam(state, pumpSyncNote, 1);
        setParam(state, pumpPhase, 0.15f);
        setParam(state, mixWet, 0.7f);
        setParam(state, outputGainDb, -1.5f);
        setParam(state, hardBypass, false);
    });

    addPreset("Chorus Crunch", [setParam](juce::ValueTree& state)
    {
        setParam(state, tapeWowDepth, 0.32f);
        setParam(state, tapeWowRateHz, 1.20f);
        setParam(state, tapeFlutterDepth, 0.24f);
        setParam(state, tapeToneLowpassHz, 11000.0f);
        setParam(state, dirtSaturationAmt, 0.4f);
        setParam(state, dirtBitDepthBits, 12);
        setParam(state, dirtSampleRateDiv, 2);
        setParam(state, pumpAmount, 0.35f);
        setParam(state, pumpSyncNote, 2);
        setParam(state, pumpPhase, 0.25f);
        setParam(state, mixWet, 0.5f);
        setParam(state, outputGainDb, 0.0f);
        setParam(state, hardBypass, false);
    });
}

juce::ValueTree DustboxProcessor::createPresetState(const std::function<void(juce::ValueTree&)>& mutator) const
{
    auto state = valueTreeState.copyState();
    mutator(state);
    return state;
}

int DustboxProcessor::findPresetIndexMatchingState(const juce::ValueTree& state) const
{
    for (size_t i = 0; i < factoryPresets.size(); ++i)
    {
        if (state.isEquivalentTo(factoryPresets[i].state))
            return static_cast<int>(i);
    }

    return -1;
}

void DustboxProcessor::publishMeterReadings(const juce::AudioBuffer<float>& buffer,
                                            std::array<MeterReadings, meterChannelCount>& storage,
                                            int numChannels,
                                            int numSamples)
{
    const int channelsToProcess = juce::jmin(static_cast<int>(meterChannelCount), numChannels);
    const float invSamples = numSamples > 0 ? 1.0f / static_cast<float>(numSamples) : 0.0f;

    for (int channel = 0; channel < channelsToProcess; ++channel)
    {
        const auto* data = buffer.getReadPointer(channel);
        float peak = 0.0f;
        double sumSquares = 0.0;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float sampleValue = data[sample];
            const float absValue = std::abs(sampleValue);
            peak = juce::jmax(peak, absValue);
            sumSquares += static_cast<double>(sampleValue) * static_cast<double>(sampleValue);
        }

        const float rms = (numSamples > 0) ? static_cast<float>(std::sqrt(sumSquares * invSamples)) : 0.0f;

        auto& readings = storage[static_cast<size_t>(channel)];
        readings.peak.store(peak, std::memory_order_relaxed);
        readings.rms.store(rms, std::memory_order_relaxed);
        readings.clip.store(peak >= 0.999f, std::memory_order_relaxed);
    }

    for (int channel = channelsToProcess; channel < static_cast<int>(meterChannelCount); ++channel)
    {
        auto& readings = storage[static_cast<size_t>(channel)];
        readings.peak.store(0.0f, std::memory_order_relaxed);
        readings.rms.store(0.0f, std::memory_order_relaxed);
        readings.clip.store(false, std::memory_order_relaxed);
    }
}

size_t DustboxProcessor::getMeterChannelCount() const noexcept
{
    return meterChannelCount;
}

float DustboxProcessor::getInputPeakLevel(size_t channel) const noexcept
{
    if (channel >= meterChannelCount)
        return 0.0f;

    return inputMeterValues[channel].peak.load(std::memory_order_relaxed);
}

float DustboxProcessor::getInputRmsLevel(size_t channel) const noexcept
{
    if (channel >= meterChannelCount)
        return 0.0f;

    return inputMeterValues[channel].rms.load(std::memory_order_relaxed);
}

bool DustboxProcessor::getInputClipFlag(size_t channel) const noexcept
{
    if (channel >= meterChannelCount)
        return false;

    return inputMeterValues[channel].clip.load(std::memory_order_relaxed);
}

float DustboxProcessor::getOutputPeakLevel(size_t channel) const noexcept
{
    if (channel >= meterChannelCount)
        return 0.0f;

    return outputMeterValues[channel].peak.load(std::memory_order_relaxed);
}

float DustboxProcessor::getOutputRmsLevel(size_t channel) const noexcept
{
    if (channel >= meterChannelCount)
        return 0.0f;

    return outputMeterValues[channel].rms.load(std::memory_order_relaxed);
}

bool DustboxProcessor::getOutputClipFlag(size_t channel) const noexcept
{
    if (channel >= meterChannelCount)
        return false;

    return outputMeterValues[channel].clip.load(std::memory_order_relaxed);
}
}

