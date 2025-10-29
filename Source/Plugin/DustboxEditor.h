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

namespace dustbox
{
class DustboxEditor : public juce::AudioProcessorEditor
{
public:
    explicit DustboxEditor(DustboxProcessor&);
    ~DustboxEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DustboxProcessor& processor;

    ui::GenericControlGroup tapeGroup;
    ui::GenericControlGroup dirtGroup;
    ui::GenericControlGroup pumpGroup;
    ui::GenericControlGroup globalGroup;
};
} // namespace dustbox

