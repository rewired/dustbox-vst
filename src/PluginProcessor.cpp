#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace dustbox
{
DustboxAudioProcessor::DustboxAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void DustboxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void DustboxAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DustboxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}
#endif

void DustboxAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    const auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < static_cast<int>(juce::jmin(totalNumInputChannels, totalNumOutputChannels)); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        const auto* inputData = buffer.getReadPointer(channel);
        if (channelData != inputData)
            juce::FloatVectorOperations::copy(channelData, inputData, numSamples);
    }
}

juce::AudioProcessorEditor* DustboxAudioProcessor::createEditor()
{
    return new DustboxAudioProcessorEditor(*this);
}

void DustboxAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    destData.setSize(0);
}

void DustboxAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DustboxAudioProcessor();
}

} // namespace dustbox

