/*
  ==============================================================================
  File: GenericControls.cpp
  Responsibility: Implement minimal grouped UI helpers for Dustbox parameters.
  Assumptions: Layout remains simple and resizes proportionally.
  TODO: Replace with bespoke design assets in future iterations.
  ==============================================================================
*/

#include "GenericControls.h"

namespace dustbox::ui
{
namespace
{
constexpr int groupMargin = 12;
constexpr int groupHeaderOffset = 24;
}

GroupContainer::GroupContainer(juce::String title)
{
    group.setText(title);
    addAndMakeVisible(group);
}

void GroupContainer::setTitle(juce::String title)
{
    group.setText(title);
}

juce::Rectangle<int> GroupContainer::getContentBounds() const noexcept
{
    auto bounds = getLocalBounds().reduced(groupMargin);
    bounds.removeFromTop(groupHeaderOffset);
    return bounds;
}

void GroupContainer::resized()
{
    group.setBounds(getLocalBounds());
}

LabeledSlider::LabeledSlider(juce::String labelText)
{
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 22);

    addAndMakeVisible(label);
    addAndMakeVisible(slider);
}

void LabeledSlider::setLabelText(juce::String labelText)
{
    label.setText(labelText, juce::dontSendNotification);
}

void LabeledSlider::resized()
{
    auto bounds = getLocalBounds();
    auto labelArea = bounds.removeFromTop(22);
    label.setBounds(labelArea);
    slider.setBounds(bounds);
}

LabeledComboBox::LabeledComboBox(juce::String labelText)
{
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);

    comboBox.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(label);
    addAndMakeVisible(comboBox);
}

void LabeledComboBox::setLabelText(juce::String labelText)
{
    label.setText(labelText, juce::dontSendNotification);
}

void LabeledComboBox::resized()
{
    auto bounds = getLocalBounds();
    auto labelArea = bounds.removeFromTop(22);
    label.setBounds(labelArea);
    comboBox.setBounds(bounds.reduced(0, 4));
}

LabeledToggleButton::LabeledToggleButton(juce::String buttonText)
{
    button.setButtonText(buttonText);
    addAndMakeVisible(button);
}

void LabeledToggleButton::resized()
{
    button.setBounds(getLocalBounds().reduced(4));
}

void LevelMeter::setLevels(float peakProportion, float rmsProportion, bool clipFlag) noexcept
{
    peak = juce::jlimit(0.0f, 1.0f, peakProportion);
    rms = juce::jlimit(0.0f, 1.0f, rmsProportion);
    clip = clipFlag;
    repaint();
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(bounds, 3.0f);

    auto meterArea = bounds.reduced(4.0f);

    if (clip)
    {
        juce::Rectangle<float> clipRect(meterArea.getX(), meterArea.getY(), meterArea.getWidth(), 4.0f);
        g.setColour(juce::Colours::red.withAlpha(0.8f));
        g.fillRect(clipRect);
    }

    auto peakHeight = meterArea.getHeight() * peak;
    juce::Rectangle<float> peakRect = meterArea;
    peakRect.removeFromTop(meterArea.getHeight() - peakHeight);

    g.setColour(juce::Colours::lightgreen);
    g.fillRect(peakRect);

    auto rmsHeight = meterArea.getHeight() * rms;
    juce::Rectangle<float> rmsRect = meterArea;
    rmsRect.removeFromTop(meterArea.getHeight() - rmsHeight);
    rmsRect.reduce(meterArea.getWidth() * 0.35f, 0.0f);

    g.setColour(juce::Colours::yellow.withAlpha(0.8f));
    g.fillRect(rmsRect);

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

void HostTempoDisplay::setTempo(double bpmValue, juce::String divisionLabel, double phaseValue) noexcept
{
    bpm = bpmValue;
    division = std::move(divisionLabel);
    phase = juce::jlimit(0.0, 1.0, phaseValue);
    repaint();
}

void HostTempoDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colours::white);
    auto textArea = bounds.reduced(6.0f);
    auto headerArea = textArea.removeFromTop(20.0f);

    juce::String text;
    text << juce::String(bpm, 1) << " BPM • " << division << " • Phase " << juce::String(phase, 2);
    g.drawFittedText(text, headerArea.toNearestInt(), juce::Justification::centred, 1);

    auto barArea = textArea.reduced(2.0f);
    g.setColour(juce::Colours::darkgrey);
    g.fillRoundedRectangle(barArea, 3.0f);

    auto fillWidth = barArea.getWidth() * static_cast<float>(phase);
    juce::Rectangle<float> fillRect = barArea.withWidth(fillWidth);
    g.setColour(juce::Colours::orange);
    g.fillRoundedRectangle(fillRect, 3.0f);

    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawRoundedRectangle(barArea, 3.0f, 1.0f);
}
} // namespace dustbox::ui

