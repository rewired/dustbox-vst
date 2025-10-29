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
GenericControlGroup::GenericControlGroup(juce::AudioProcessorValueTreeState& state,
                                         juce::String groupName,
                                         std::vector<ControlDefinition> definitions)
    : valueTreeState(state)
{
    addAndMakeVisible(group);
    group.setText(groupName);

    for (const auto& def : definitions)
    {
        switch (def.type)
        {
            case ControlType::Slider:
            {
                auto label = std::make_unique<juce::Label>();
                label->setText(def.label, juce::dontSendNotification);
                label->setJustificationType(juce::Justification::centred);

                auto slider = std::make_unique<juce::Slider>();
                slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
                slider->setName(def.label);

                addAndMakeVisible(*slider);
                addAndMakeVisible(*label);
                label->attachToComponent(slider.get(), false);

                sliderAttachments.push_back(std::make_unique<SliderAttachment>(valueTreeState, def.parameterID, *slider));
                controlOrder.push_back(slider.get());
                sliders.push_back(std::move(slider));
                labels.push_back(std::move(label));
                break;
            }
            case ControlType::Choice:
            {
                auto label = std::make_unique<juce::Label>();
                label->setText(def.label, juce::dontSendNotification);
                label->setJustificationType(juce::Justification::centredLeft);

                auto combo = std::make_unique<juce::ComboBox>();
                combo->setName(def.label);

                addAndMakeVisible(*combo);
                addAndMakeVisible(*label);
                label->attachToComponent(combo.get(), true);

                comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(valueTreeState, def.parameterID, *combo));
                controlOrder.push_back(combo.get());
                comboBoxes.push_back(std::move(combo));
                labels.push_back(std::move(label));
                break;
            }
            case ControlType::Button:
            {
                auto button = std::make_unique<juce::ToggleButton>(def.label);
                addAndMakeVisible(*button);
                buttonAttachments.push_back(std::make_unique<ButtonAttachment>(valueTreeState, def.parameterID, *button));
                controlOrder.push_back(button.get());
                buttons.push_back(std::move(button));
                break;
            }
        }
    }
}

void GenericControlGroup::resized()
{
    auto bounds = getLocalBounds();
    group.setBounds(bounds);
    auto inner = bounds.reduced(12);
    inner.removeFromTop(16);

    juce::FlexBox flexBox;
    flexBox.flexDirection = juce::FlexBox::Direction::row;
    flexBox.flexWrap = juce::FlexBox::Wrap::wrap;
    flexBox.alignContent = juce::FlexBox::AlignContent::flexStart;

    juce::Array<juce::FlexItem> items;
    for (auto* component : controlOrder)
    {
        juce::FlexItem item(*component);
        item.margin = juce::FlexItem::Margin(6.0f);
        if (dynamic_cast<juce::Slider*>(component) != nullptr)
            item = item.withMinWidth(120.0f).withMinHeight(140.0f);
        else
            item = item.withMinWidth(150.0f).withMinHeight(50.0f);
        items.add(item);
    }

    flexBox.items = items;
    flexBox.performLayout(inner);
}
} // namespace dustbox::ui

