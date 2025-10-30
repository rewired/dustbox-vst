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

enum class NoiseRouting
{
    PreTape = 0,
    PostTape,
    Parallel
};

struct ProcessorSuspender
{
    explicit ProcessorSuspender(DustboxProcessor& processorIn) : processor(processorIn)
    {
        if (! processor.isSuspended())
        {
            processor.suspendProcessing(true);
            suspended = true;
        }
    }

    ~ProcessorSuspender()
    {
        if (suspended)
            processor.suspendProcessing(false);
    }

private:
    DustboxProcessor& processor;
    bool suspended { false };
};
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
    noiseModule.prepare(sampleRate, samplesPerBlock, numChannels);
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
    noiseModule.reset();
    dirtModule.reset();
    pumpModule.reset();

    bypassTransitionActive = false;
}

void DustboxProcessor::releaseResources()
{
    dryBuffer.setSize(0, 0);
    noiseModule.reset();
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

    noiseModule.generate(numSamples);
    const auto& noise = noiseModule.getNoiseBuffer();
    const auto noiseChannels = juce::jmin(noise.getNumChannels(), buffer.getNumChannels());
    const auto noiseRouting = static_cast<NoiseRouting>(cachedParameters.noiseRoutingIndex);

    if (noiseRouting == NoiseRouting::PreTape)
    {
        for (int channel = 0; channel < noiseChannels; ++channel)
            buffer.addFrom(channel, 0, noise, channel, 0, numSamples);
    }

    tapeModule.processBlock(buffer, numSamples);

    if (noiseRouting == NoiseRouting::PostTape)
    {
        for (int channel = 0; channel < noiseChannels; ++channel)
            buffer.addFrom(channel, 0, noise, channel, 0, numSamples);
    }

    dirtModule.processBlock(buffer, numSamples);
    pumpModule.processBlock(buffer, numSamples);

    wetMixSmoother.setTarget(cachedParameters.wetMix);
    outputGainSmoother.setTarget(cachedParameters.outputGain);

    jassert(totalNumInputChannels <= static_cast<int>(maxProcessChannels));
    std::array<const float*, maxProcessChannels> dryPointers {};
    std::array<float*, maxProcessChannels> wetPointers {};
    std::array<const float*, maxProcessChannels> noisePointers {};

    const bool noiseParallel = noiseRouting == NoiseRouting::Parallel;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        dryPointers[static_cast<size_t>(channel)] = dryBuffer.getReadPointer(channel);
        wetPointers[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);
        if (noiseParallel && channel < noiseChannels)
            noisePointers[static_cast<size_t>(channel)] = noise.getReadPointer(channel);
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
            auto mixed = (drySample * gains.dry + wetSample * gains.wet) * outputGain;

            if (noiseParallel && noisePointers[channelIndex] != nullptr)
                mixed += noisePointers[channelIndex][sample] * outputGain;

            wetPointers[channelIndex][sample] = mixed;
        }
    }
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
    const auto& preset = factoryPresets[static_cast<size_t>(clamped)];
    const bool needsUpdate = ! preset.state.isEquivalentTo(valueTreeState.state);

    if (needsUpdate)
    {
        ProcessorSuspender suspend(*this);
        valueTreeState.replaceState(preset.state.createCopy());
    }

    currentProgramIndex = clamped;

    if (needsUpdate)
        updateParameters();

    updateHostDisplay();
}

const juce::String DustboxProcessor::getProgramName(int index)
{
    if (index < 0 || index >= getNumPrograms() || factoryPresets.empty())
        return {};

    return factoryPresets[static_cast<size_t>(index)].name;
}

void DustboxProcessor::changeProgramName(int, const juce::String&)
{
    // Factory presets are immutable; hosts may request a rename but we ignore it.
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
    tapeModule.setParameters(cachedParameters.tapeParams);

    cachedParameters.noiseParams.levelDb = getFloat(params::ids::tapeNoiseLevelDb);
    cachedParameters.noiseRoutingIndex = juce::jlimit(0, 2, getChoice(params::ids::noiseRouting));
    noiseModule.setParameters(cachedParameters.noiseParams);

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

void DustboxProcessor::initialiseFactoryPresets()
{
    factoryPresets = presets::createFactoryPresets(valueTreeState);
    currentProgramIndex = factoryPresets.empty() ? 0 : juce::jlimit(0, getNumPrograms() - 1, currentProgramIndex);
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
} // namespace dustbox

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new dustbox::DustboxProcessor();
}

