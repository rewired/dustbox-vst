/*
  ==============================================================================
  File: DustboxEditor.h
  Responsibility: Declare the Dustbox editor providing grouped generic controls.
  Assumptions: Editor owns lightweight control groups backed by APVTS.
  TODO: Replace with custom skin and interactions in future sprints.
  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "DustboxProcessor.h"
#include "../Ui/GenericControls.h"

#include <array>
#include <atomic>

namespace dustbox
{
class DustboxEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DustboxEditor(DustboxProcessor&);
    ~DustboxEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void initialiseControls();
    void initialiseAttachments();
    void refreshPresetCombo();
    void updateMeters();
    void updateTempoDisplay();
    void layoutGroupFlex(ui::GroupContainer& group,
                         const juce::Array<juce::Component*>& components,
                         juce::Rectangle<int> bounds = {});
    static float amplitudeToDisplayProportion(float amplitude);

    DustboxProcessor& processor;

    ui::GroupContainer tapeGroup;
    ui::GroupContainer dirtGroup;
    ui::GroupContainer pumpGroup;
    ui::GroupContainer globalGroup;

    ui::LabeledSlider tapeWowDepth;
    ui::LabeledSlider tapeWowRate;
    ui::LabeledSlider tapeFlutterDepth;
    ui::LabeledSlider tapeToneLowpass;
    ui::LabeledSlider tapeNoiseLevel;
    ui::LabeledComboBox noiseRouting;

    ui::LabeledSlider dirtSaturation;
    ui::LabeledSlider dirtBitDepth;
    ui::LabeledSlider dirtSampleRateDiv;

    ui::LabeledSlider pumpAmount;
    ui::LabeledComboBox pumpSyncNote;
    ui::LabeledSlider pumpPhase;
    ui::HostTempoDisplay tempoDisplay;

    ui::LabeledSlider mixWet;
    ui::LabeledSlider outputGain;
    ui::LabeledToggleButton hardBypass;
    ui::LabeledComboBox presetSelector;

    ui::LevelMeter inputMeterLeft;
    ui::LevelMeter inputMeterRight;
    ui::LevelMeter outputMeterLeft;
    ui::LevelMeter outputMeterRight;
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;
    juce::Label clipIndicator;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> tapeWowDepthAttachment;
    std::unique_ptr<SliderAttachment> tapeWowRateAttachment;
    std::unique_ptr<SliderAttachment> tapeFlutterAttachment;
    std::unique_ptr<SliderAttachment> tapeToneAttachment;
    std::unique_ptr<SliderAttachment> tapeNoiseLevelAttachment;
    std::unique_ptr<ComboBoxAttachment> noiseRoutingAttachment;

    std::unique_ptr<SliderAttachment> dirtSaturationAttachment;
    std::unique_ptr<SliderAttachment> dirtBitDepthAttachment;
    std::unique_ptr<SliderAttachment> dirtSampleRateAttachment;

    std::unique_ptr<SliderAttachment> pumpAmountAttachment;
    std::unique_ptr<ComboBoxAttachment> pumpSyncAttachment;
    std::unique_ptr<SliderAttachment> pumpPhaseAttachment;

    std::unique_ptr<SliderAttachment> mixWetAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<ButtonAttachment> hardBypassAttachment;

    std::atomic<float>* pumpSyncParameter { nullptr };

    std::array<float, 2> inputPeakDisplay { 0.0f, 0.0f };
    std::array<float, 2> inputRmsDisplay { 0.0f, 0.0f };
    std::array<float, 2> outputPeakDisplay { 0.0f, 0.0f };
    std::array<float, 2> outputRmsDisplay { 0.0f, 0.0f };
    int clipHoldCounter { 0 };
    bool updatingPresetSelection { false };
};
} // namespace dustbox

