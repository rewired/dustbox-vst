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

#include <vector>

namespace dustbox::ui
{
class GenericControlGroup : public juce::Component
{
public:
    enum class ControlType
    {
        Slider,
        Button,
        Choice
    };

    struct ControlDefinition
    {
        juce::String parameterID;
        juce::String label;
        ControlType type;
    };

    GenericControlGroup(juce::AudioProcessorValueTreeState& state,
                        juce::String groupName,
                        std::vector<ControlDefinition> definitions);

    void resized() override;

private:
    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::GroupComponent group;

    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::ComboBox>> comboBoxes;
    std::vector<std::unique_ptr<juce::ToggleButton>> buttons;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<juce::Component*> controlOrder;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
};
} // namespace dustbox::ui

