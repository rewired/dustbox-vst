/*
  ==============================================================================
  File: DustboxEditor.cpp
  Responsibility: Implement the minimal grouped generic editor for Dustbox.
  Assumptions: Layout remains simple and relies on generic JUCE controls.
  TODO: Replace with custom artwork once design direction is finalised.
  ==============================================================================
*/

#include "DustboxEditor.h"

#include "../Parameters/ParameterIDs.h"

namespace dustbox
{
DustboxEditor::DustboxEditor(DustboxProcessor& p)
    : juce::AudioProcessorEditor(&p)
    , processor(p)
    , tapeGroup(p.getValueTreeState(),
                "AGE",
                {
                    { params::ids::tapeWowDepth, "Wow Depth", ui::GenericControlGroup::ControlType::Slider },
                    { params::ids::tapeWowRateHz, "Wow Rate", ui::GenericControlGroup::ControlType::Slider },
                    { params::ids::tapeFlutterDepth, "Flutter", ui::GenericControlGroup::ControlType::Slider },
                    { params::ids::tapeToneLowpassHz, "Tone", ui::GenericControlGroup::ControlType::Slider },
                    { params::ids::tapeNoiseLevelDb, "Noise", ui::GenericControlGroup::ControlType::Slider },
                    { params::ids::tapeNoiseRoute, "Noise Route", ui::GenericControlGroup::ControlType::Choice },
                })
    , dirtGroup(p.getValueTreeState(),
                "DIRT",
                {
                    { params::ids::dirtSaturationAmt, "Saturation", ui::GenericControlGroup::ControlType::Slider },
                    { params::ids::dirtBitDepthBits, "Bit Depth", ui::GenericControlGroup::ControlType::Slider },
                    { params::ids::dirtSampleRateDiv, "Rate Div", ui::GenericControlGroup::ControlType::Slider },
                })
    , pumpGroup(p.getValueTreeState(),
                "PUMP",
                {
                    { params::ids::pumpAmount, "Amount", ui::GenericControlGroup::ControlType::Slider },
                    { params::ids::pumpSyncNote, "Sync", ui::GenericControlGroup::ControlType::Choice },
                    { params::ids::pumpPhase, "Phase", ui::GenericControlGroup::ControlType::Slider },
                })
    , globalGroup(p.getValueTreeState(),
                  "GLOBAL",
                  {
                      { params::ids::mixWet, "Mix", ui::GenericControlGroup::ControlType::Slider },
                      { params::ids::outputGainDb, "Output", ui::GenericControlGroup::ControlType::Slider },
                      { params::ids::hardBypass, "Bypass", ui::GenericControlGroup::ControlType::Button },
                  })
{
    addAndMakeVisible(tapeGroup);
    addAndMakeVisible(dirtGroup);
    addAndMakeVisible(pumpGroup);
    addAndMakeVisible(globalGroup);

    setSize(760, 520);
}

void DustboxEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.9f));
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("DUSTBOX", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);
}

void DustboxEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(40);

    auto topRow = area.removeFromTop(area.getHeight() / 2);
    auto bottomRow = area;

    auto leftTop = topRow.removeFromLeft(topRow.getWidth() / 2);
    tapeGroup.setBounds(leftTop.reduced(8));
    dirtGroup.setBounds(topRow.reduced(8));

    auto leftBottom = bottomRow.removeFromLeft(bottomRow.getWidth() / 2);
    pumpGroup.setBounds(leftBottom.reduced(8));
    globalGroup.setBounds(bottomRow.reduced(8));
}
} // namespace dustbox

