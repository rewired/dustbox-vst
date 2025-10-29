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

#include <algorithm>
#include <array>
#include <cmath>

namespace dustbox
{
namespace
{
constexpr int clipHoldFrames = 20;
constexpr float meterSmoothing = 0.35f;

const std::array<juce::String, 3> noteDivisionLabels {
    juce::String(juce::CharPointer_UTF8("\xC2\xBC")), // ¼
    juce::String(juce::CharPointer_UTF8("\xE2\x85\x9B")), // ⅛
    juce::String(juce::CharPointer_UTF8("1\xE2\x81\x8416")) // 1⁄16
};
} // namespace

DustboxEditor::DustboxEditor(DustboxProcessor& p)
    : juce::AudioProcessorEditor(&p)
    , processor(p)
    , tapeGroup("TAPE")
    , dirtGroup("DIRT")
    , pumpGroup("PUMP")
    , globalGroup("GLOBAL")
    , tapeWowDepth("Wow Depth")
    , tapeWowRate("Wow Rate")
    , tapeFlutterDepth("Flutter")
    , tapeToneLowpass("Tone")
    , tapeNoiseLevel("Noise")
    , tapeNoiseRoute("Noise Route")
    , dirtSaturation("Saturation")
    , dirtBitDepth("Bit Depth")
    , dirtSampleRateDiv("Rate Div")
    , pumpAmount("Amount")
    , pumpSyncNote("Sync")
    , pumpPhase("Phase")
    , mixWet("Mix")
    , outputGain("Output")
    , hardBypass("Hard Bypass")
    , presetSelector("Preset")
{
    initialiseControls();
    initialiseAttachments();

    addAndMakeVisible(tapeGroup);
    tapeGroup.addAndMakeVisible(tapeWowDepth);
    tapeGroup.addAndMakeVisible(tapeWowRate);
    tapeGroup.addAndMakeVisible(tapeFlutterDepth);
    tapeGroup.addAndMakeVisible(tapeToneLowpass);
    tapeGroup.addAndMakeVisible(tapeNoiseLevel);
    tapeGroup.addAndMakeVisible(tapeNoiseRoute);

    addAndMakeVisible(dirtGroup);
    dirtGroup.addAndMakeVisible(dirtSaturation);
    dirtGroup.addAndMakeVisible(dirtBitDepth);
    dirtGroup.addAndMakeVisible(dirtSampleRateDiv);

    addAndMakeVisible(pumpGroup);
    pumpGroup.addAndMakeVisible(pumpAmount);
    pumpGroup.addAndMakeVisible(pumpSyncNote);
    pumpGroup.addAndMakeVisible(pumpPhase);
    pumpGroup.addAndMakeVisible(tempoDisplay);

    addAndMakeVisible(globalGroup);
    globalGroup.addAndMakeVisible(mixWet);
    globalGroup.addAndMakeVisible(outputGain);
    globalGroup.addAndMakeVisible(hardBypass);
    globalGroup.addAndMakeVisible(presetSelector);
    globalGroup.addAndMakeVisible(inputMeterLabel);
    globalGroup.addAndMakeVisible(outputMeterLabel);
    globalGroup.addAndMakeVisible(clipIndicator);
    globalGroup.addAndMakeVisible(inputMeterLeft);
    globalGroup.addAndMakeVisible(inputMeterRight);
    globalGroup.addAndMakeVisible(outputMeterLeft);
    globalGroup.addAndMakeVisible(outputMeterRight);

    refreshPresetCombo();

    pumpSyncParameter = processor.getValueTreeState().getRawParameterValue(params::ids::pumpSyncNote);

    setResizable(true, true);
    setResizeLimits(720, 560, 1280, 960);
    setSize(820, 640);

    startTimerHz(30);
}

DustboxEditor::~DustboxEditor()
{
    stopTimer();
}

void DustboxEditor::initialiseControls()
{
    auto percentText = [](double value) { return juce::String(value * 100.0, 1) + " %"; };
    auto percentToValue = [](const juce::String& text) { return text.getDoubleValue() / 100.0; };

    auto configurePercentSlider = [&](ui::LabeledSlider& control)
    {
        auto& slider = control.getSlider();
        slider.textFromValueFunction = percentText;
        slider.valueFromTextFunction = percentToValue;
    };

    configurePercentSlider(tapeWowDepth);
    configurePercentSlider(tapeFlutterDepth);
    configurePercentSlider(dirtSaturation);
    configurePercentSlider(pumpAmount);
    configurePercentSlider(mixWet);

    auto& wowRate = tapeWowRate.getSlider();
    wowRate.setNumDecimalPlacesToDisplay(2);
    wowRate.setTextValueSuffix(" Hz");

    auto& tone = tapeToneLowpass.getSlider();
    tone.setNumDecimalPlacesToDisplay(0);
    tone.setTextValueSuffix(" Hz");

    auto& noise = tapeNoiseLevel.getSlider();
    noise.setNumDecimalPlacesToDisplay(1);
    noise.setTextValueSuffix(" dB");

    auto& bitDepth = dirtBitDepth.getSlider();
    bitDepth.setNumDecimalPlacesToDisplay(0);
    bitDepth.textFromValueFunction = [](double value) { return juce::String(static_cast<int>(std::round(value))) + " bit"; };
    bitDepth.valueFromTextFunction = [](const juce::String& text) { return text.getDoubleValue(); };

    auto& rateDiv = dirtSampleRateDiv.getSlider();
    rateDiv.setNumDecimalPlacesToDisplay(0);
    rateDiv.textFromValueFunction = [](double value) { return juce::String(static_cast<int>(std::round(value))) + "x"; };
    rateDiv.valueFromTextFunction = [](const juce::String& text) { return text.getDoubleValue(); };

    auto& phase = pumpPhase.getSlider();
    const juce::String degreeSymbol { juce::CharPointer_UTF8("\xC2\xB0") };
    phase.textFromValueFunction = [degreeSymbol](double value) { return juce::String(value * 360.0, 1) + degreeSymbol; };
    phase.valueFromTextFunction = [](const juce::String& text) { return text.getDoubleValue() / 360.0; };

    auto& output = outputGain.getSlider();
    output.setNumDecimalPlacesToDisplay(1);
    output.setTextValueSuffix(" dB");

    auto& noiseRouteCombo = tapeNoiseRoute.getComboBox();
    noiseRouteCombo.addItem("Wet Pre Pump", 1);
    noiseRouteCombo.addItem("Wet Post Pump", 2);
    noiseRouteCombo.addItem("Post Mix", 3);

    auto& pumpSyncCombo = pumpSyncNote.getComboBox();
    pumpSyncCombo.addItem(noteDivisionLabels[0], 1);
    pumpSyncCombo.addItem(noteDivisionLabels[1], 2);
    pumpSyncCombo.addItem(noteDivisionLabels[2], 3);

    auto& presetCombo = presetSelector.getComboBox();
    presetCombo.setTextWhenNothingSelected("Factory Presets");
    presetCombo.onChange = [this]
    {
        if (updatingPresetSelection)
            return;

        const int selectedIndex = presetSelector.getComboBox().getSelectedItemIndex();
        if (selectedIndex >= 0)
            processor.setCurrentProgram(selectedIndex);
    };

    hardBypass.getButton().setClickingTogglesState(true);

    auto setupMeterLabel = [](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    };

    setupMeterLabel(inputMeterLabel, "INPUT");
    setupMeterLabel(outputMeterLabel, "OUTPUT");

    clipIndicator.setColour(juce::Label::textColourId, juce::Colours::red);
    clipIndicator.setJustificationType(juce::Justification::centredRight);
    clipIndicator.setFont(juce::Font(14.0f, juce::Font::bold));
}

void DustboxEditor::initialiseAttachments()
{
    auto& state = processor.getValueTreeState();

    tapeWowDepthAttachment = std::make_unique<SliderAttachment>(state, params::ids::tapeWowDepth, tapeWowDepth.getSlider());
    tapeWowRateAttachment = std::make_unique<SliderAttachment>(state, params::ids::tapeWowRateHz, tapeWowRate.getSlider());
    tapeFlutterAttachment = std::make_unique<SliderAttachment>(state, params::ids::tapeFlutterDepth, tapeFlutterDepth.getSlider());
    tapeToneAttachment = std::make_unique<SliderAttachment>(state, params::ids::tapeToneLowpassHz, tapeToneLowpass.getSlider());
    tapeNoiseLevelAttachment = std::make_unique<SliderAttachment>(state, params::ids::tapeNoiseLevelDb, tapeNoiseLevel.getSlider());
    tapeNoiseRouteAttachment = std::make_unique<ComboBoxAttachment>(state, params::ids::tapeNoiseRoute, tapeNoiseRoute.getComboBox());

    dirtSaturationAttachment = std::make_unique<SliderAttachment>(state, params::ids::dirtSaturationAmt, dirtSaturation.getSlider());
    dirtBitDepthAttachment = std::make_unique<SliderAttachment>(state, params::ids::dirtBitDepthBits, dirtBitDepth.getSlider());
    dirtSampleRateAttachment = std::make_unique<SliderAttachment>(state, params::ids::dirtSampleRateDiv, dirtSampleRateDiv.getSlider());

    pumpAmountAttachment = std::make_unique<SliderAttachment>(state, params::ids::pumpAmount, pumpAmount.getSlider());
    pumpSyncAttachment = std::make_unique<ComboBoxAttachment>(state, params::ids::pumpSyncNote, pumpSyncNote.getComboBox());
    pumpPhaseAttachment = std::make_unique<SliderAttachment>(state, params::ids::pumpPhase, pumpPhase.getSlider());

    mixWetAttachment = std::make_unique<SliderAttachment>(state, params::ids::mixWet, mixWet.getSlider());
    outputGainAttachment = std::make_unique<SliderAttachment>(state, params::ids::outputGainDb, outputGain.getSlider());
    hardBypassAttachment = std::make_unique<ButtonAttachment>(state, params::ids::hardBypass, hardBypass.getButton());
}

void DustboxEditor::refreshPresetCombo()
{
    auto& combo = presetSelector.getComboBox();
    const int numPrograms = processor.getNumPrograms();

    const bool needsRefresh = combo.getNumItems() != numPrograms;
    if (needsRefresh)
    {
        combo.clear(juce::dontSendNotification);
        for (int index = 0; index < numPrograms; ++index)
            combo.addItem(processor.getProgramName(index), index + 1);
    }

    const int targetIndex = numPrograms > 0 ? processor.getCurrentProgram() : -1;

    if (targetIndex >= 0 && combo.getSelectedItemIndex() != targetIndex)
    {
        const juce::ScopedValueSetter<bool> scoped(updatingPresetSelection, true);
        combo.setSelectedItemIndex(targetIndex, juce::dontSendNotification);
    }
}

void DustboxEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(24, 24, 24));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("DUSTBOX", 16, 8, getWidth() - 32, 28, juce::Justification::centred);
}

void DustboxEditor::resized()
{
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(40);

    constexpr int groupGap = 12;
    auto remaining = bounds;

    auto assignGroup = [&](ui::GroupContainer& group, int remainingGroups)
    {
        const int gap = remainingGroups > 0 ? groupGap : 0;
        const int groupHeight = (remaining.getHeight() - gap * remainingGroups) / (remainingGroups + 1);
        auto area = remaining.removeFromTop(groupHeight);
        remaining.removeFromTop(gap);
        group.setBounds(area);
    };

    assignGroup(tapeGroup, 3);
    assignGroup(dirtGroup, 2);
    assignGroup(pumpGroup, 1);
    assignGroup(globalGroup, 0);

    layoutGroupFlex(tapeGroup, { &tapeWowDepth, &tapeWowRate, &tapeFlutterDepth, &tapeToneLowpass, &tapeNoiseLevel, &tapeNoiseRoute });
    layoutGroupFlex(dirtGroup, { &dirtSaturation, &dirtBitDepth, &dirtSampleRateDiv });
    layoutGroupFlex(pumpGroup, { &pumpAmount, &pumpSyncNote, &pumpPhase, &tempoDisplay });

    auto globalContent = globalGroup.getContentBounds();
    const int meterWidth = juce::jlimit(160, globalContent.getWidth(), 240);
    auto meterArea = globalContent.removeFromRight(meterWidth);
    auto controlArea = globalContent;
    layoutGroupFlex(globalGroup, { &mixWet, &outputGain, &hardBypass, &presetSelector }, controlArea);

    auto meterSpacing = 10;

    auto layoutMeterPair = [&](juce::Label& label, ui::LevelMeter& left, ui::LevelMeter& right, juce::Rectangle<int> area)
    {
        auto labelArea = area.removeFromTop(22);
        label.setBounds(labelArea);
        area.removeFromTop(4);
        const int width = (area.getWidth() - meterSpacing) / 2;
        left.setBounds(area.removeFromLeft(width));
        area.removeFromLeft(meterSpacing);
        right.setBounds(area.removeFromLeft(width));
    };

    auto inputArea = meterArea.removeFromTop(meterArea.getHeight() / 2);
    layoutMeterPair(inputMeterLabel, inputMeterLeft, inputMeterRight, inputArea);

    auto outputArea = meterArea;
    auto outputLabelArea = outputArea.removeFromTop(22);
    auto clipArea = outputLabelArea.removeFromRight(60);
    clipIndicator.setBounds(clipArea);
    outputMeterLabel.setBounds(outputLabelArea);
    outputArea.removeFromTop(4);
    const int outputWidth = (outputArea.getWidth() - meterSpacing) / 2;
    outputMeterLeft.setBounds(outputArea.removeFromLeft(outputWidth));
    outputArea.removeFromLeft(meterSpacing);
    outputMeterRight.setBounds(outputArea.removeFromLeft(outputWidth));
}

void DustboxEditor::timerCallback()
{
    updateMeters();
    updateTempoDisplay();
    refreshPresetCombo();
}

void DustboxEditor::updateMeters()
{
    const auto channelCount = std::min<size_t>(processor.getMeterChannelCount(), 2);

    bool clipped = false;
    for (size_t channel = 0; channel < channelCount; ++channel)
    {
        const float inputPeak = amplitudeToDisplayProportion(processor.getInputPeakLevel(channel));
        const float inputRms = amplitudeToDisplayProportion(processor.getInputRmsLevel(channel));
        const float outputPeak = amplitudeToDisplayProportion(processor.getOutputPeakLevel(channel));
        const float outputRms = amplitudeToDisplayProportion(processor.getOutputRmsLevel(channel));

        inputPeakDisplay[channel] = inputPeakDisplay[channel] + meterSmoothing * (inputPeak - inputPeakDisplay[channel]);
        inputRmsDisplay[channel] = inputRmsDisplay[channel] + meterSmoothing * (inputRms - inputRmsDisplay[channel]);
        outputPeakDisplay[channel] = outputPeakDisplay[channel] + meterSmoothing * (outputPeak - outputPeakDisplay[channel]);
        outputRmsDisplay[channel] = outputRmsDisplay[channel] + meterSmoothing * (outputRms - outputRmsDisplay[channel]);

        const bool inputClip = processor.getInputClipFlag(channel);
        const bool outputClip = processor.getOutputClipFlag(channel);
        clipped = clipped || outputClip;

        if (channel == 0)
        {
            inputMeterLeft.setLevels(inputPeakDisplay[channel], inputRmsDisplay[channel], inputClip);
            outputMeterLeft.setLevels(outputPeakDisplay[channel], outputRmsDisplay[channel], outputClip);
        }
        else
        {
            inputMeterRight.setLevels(inputPeakDisplay[channel], inputRmsDisplay[channel], inputClip);
            outputMeterRight.setLevels(outputPeakDisplay[channel], outputRmsDisplay[channel], outputClip);
        }
    }

    for (size_t channel = channelCount; channel < 2; ++channel)
    {
        inputPeakDisplay[channel] *= 0.5f;
        inputRmsDisplay[channel] *= 0.5f;
        outputPeakDisplay[channel] *= 0.5f;
        outputRmsDisplay[channel] *= 0.5f;

        if (channel == 0)
        {
            inputMeterLeft.setLevels(0.0f, 0.0f, false);
            outputMeterLeft.setLevels(0.0f, 0.0f, false);
        }
        else
        {
            inputMeterRight.setLevels(0.0f, 0.0f, false);
            outputMeterRight.setLevels(0.0f, 0.0f, false);
        }
    }

    if (clipped)
        clipHoldCounter = clipHoldFrames;
    else if (clipHoldCounter > 0)
        --clipHoldCounter;

    clipIndicator.setText(clipHoldCounter > 0 ? "CLIP" : juce::String(), juce::dontSendNotification);
}

void DustboxEditor::updateTempoDisplay()
{
    const int divisionIndex = pumpSyncParameter != nullptr ? static_cast<int>(pumpSyncParameter->load()) : 1;
    const int clampedIndex = juce::jlimit(0, static_cast<int>(noteDivisionLabels.size()) - 1, divisionIndex);
    const auto& hostTempo = processor.getHostTempo();
    const double bpm = hostTempo.getBpm();
    const double phase = hostTempo.getPhase01(clampedIndex);
    tempoDisplay.setTempo(bpm, noteDivisionLabels[static_cast<size_t>(clampedIndex)], phase);
}

void DustboxEditor::layoutGroupFlex(ui::GroupContainer& group,
                                    const juce::Array<juce::Component*>& components,
                                    juce::Rectangle<int> bounds)
{
    juce::FlexBox flex;
    flex.flexDirection = juce::FlexBox::Direction::row;
    flex.flexWrap = juce::FlexBox::Wrap::wrap;
    flex.justifyContent = juce::FlexBox::JustifyContent::flexStart;

    for (auto* component : components)
    {
        juce::FlexItem item(*component);
        item.margin = juce::FlexItem::Margin(6.0f);

        if (dynamic_cast<ui::LabeledSlider*>(component) != nullptr)
        {
            item.minWidth = 140.0f;
            item.minHeight = 150.0f;
            item.flexGrow = 1.0f;
        }
        else if (dynamic_cast<ui::LabeledComboBox*>(component) != nullptr)
        {
            item.minWidth = 160.0f;
            item.minHeight = 70.0f;
            item.flexGrow = 1.0f;
        }
        else if (dynamic_cast<ui::LabeledToggleButton*>(component) != nullptr)
        {
            item.minWidth = 140.0f;
            item.minHeight = 60.0f;
        }
        else if (dynamic_cast<ui::HostTempoDisplay*>(component) != nullptr)
        {
            item.minWidth = 200.0f;
            item.minHeight = 90.0f;
            item.flexGrow = 1.0f;
        }
        else
        {
            item.minWidth = 100.0f;
            item.minHeight = 120.0f;
        }

        flex.items.add(item);
    }

    auto targetBounds = bounds.isEmpty() ? group.getContentBounds() : bounds;
    flex.performLayout(targetBounds);
}

float DustboxEditor::amplitudeToDisplayProportion(float amplitude)
{
    const float clamped = juce::jlimit(1.0e-5f, 8.0f, amplitude);
    const float db = juce::Decibels::gainToDecibels(clamped, -60.0f);
    return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
}
} // namespace dustbox

