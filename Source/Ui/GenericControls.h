/*
  ==============================================================================
  File: GenericControls.h
  Responsibility: Provide lightweight grouped UI components for Dustbox params.
  Assumptions: Components are used within the DustboxEditor layout.
  TODO: Replace with bespoke UI once final design is available.
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

namespace dustbox::ui
{
/** Lightweight wrapper around juce::GroupComponent providing a padded content area. */
class GroupContainer : public juce::Component
{
public:
    explicit GroupContainer(juce::String title);

    void setTitle(juce::String title);

    /** Returns the area available for laying out child controls inside the group. */
    juce::Rectangle<int> getContentBounds() const noexcept;

    void resized() override;

private:
    juce::GroupComponent group;
};

/** Slider paired with a caption above it. */
class LabeledSlider : public juce::Component
{
public:
    explicit LabeledSlider(juce::String labelText = {});

    void setLabelText(juce::String labelText);

    juce::Slider& getSlider() noexcept { return slider; }
    const juce::Slider& getSlider() const noexcept { return slider; }

    void resized() override;

private:
    juce::Label label;
    juce::Slider slider;
};

/** ComboBox with a caption above it. */
class LabeledComboBox : public juce::Component
{
public:
    explicit LabeledComboBox(juce::String labelText = {});

    void setLabelText(juce::String labelText);

    juce::ComboBox& getComboBox() noexcept { return comboBox; }
    const juce::ComboBox& getComboBox() const noexcept { return comboBox; }

    void resized() override;

private:
    juce::Label label;
    juce::ComboBox comboBox;
};

/** Toggle button centred within its bounds. */
class LabeledToggleButton : public juce::Component
{
public:
    explicit LabeledToggleButton(juce::String buttonText = {});

    juce::ToggleButton& getButton() noexcept { return button; }
    const juce::ToggleButton& getButton() const noexcept { return button; }

    void resized() override;

private:
    juce::ToggleButton button;
};

/** Simple peak/RMS meter with optional clip indication. */
class LevelMeter : public juce::Component
{
public:
    void setLevels(float peakProportion, float rmsProportion, bool clipFlag) noexcept;
    void paint(juce::Graphics& g) override;

private:
    float peak { 0.0f };
    float rms { 0.0f };
    bool clip { false };
};

/** Displays BPM, sync division, and phase with a tiny progress indicator. */
class HostTempoDisplay : public juce::Component
{
public:
    void setTempo(double bpmValue, juce::String divisionLabel, double phaseValue) noexcept;
    void paint(juce::Graphics& g) override;

private:
    double bpm { 120.0 };
    juce::String division { "1/4" };
    double phase { 0.0 };
};
} // namespace dustbox::ui

