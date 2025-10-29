#pragma once

/**
 * Dustbox — Minimal editor scaffold.
 */

#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace dustbox
{
class DustboxAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit DustboxAudioProcessorEditor(DustboxAudioProcessor&);
    ~DustboxAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DustboxAudioProcessor& processor;
    juce::Label headline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DustboxAudioProcessorEditor)
};

} // namespace dustbox

