#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace dustbox
{
DustboxAudioProcessorEditor::DustboxAudioProcessorEditor(DustboxAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    setSize(360, 200);

    headline.setText("Dustbox — MVP Skeleton", juce::dontSendNotification);
    headline.setJustificationType(juce::Justification::centred);
    headline.setFont(juce::Font(18.0f, juce::Font::bold));
    addAndMakeVisible(headline);
}

void DustboxAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);
}

void DustboxAudioProcessorEditor::resized()
{
    headline.setBounds(getLocalBounds().reduced(20));
}

} // namespace dustbox

