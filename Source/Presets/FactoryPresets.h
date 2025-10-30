/*
  ==============================================================================
  File: FactoryPresets.h
  Responsibility: Declare built-in Dustbox factory presets and helpers for
                  constructing APVTS snapshot states.
  Assumptions: AudioProcessorValueTreeState owns all parameter IDs listed in
               ParameterIDs.h with stable identifiers.
  Notes: Preset states must be deterministic to guarantee host-visible program
         changes serialise identically to manual parameter edits.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <initializer_list>
#include <utility>
#include <vector>

namespace dustbox::presets
{
struct FactoryPreset
{
    juce::String name;
    juce::ValueTree state;
};

using ParameterSetting = std::pair<juce::Identifier, juce::var>;

juce::ValueTree makeStateFromMap(const juce::AudioProcessorValueTreeState& apvts,
                                 std::initializer_list<ParameterSetting> settings);

std::vector<FactoryPreset> createFactoryPresets(const juce::AudioProcessorValueTreeState& apvts);

} // namespace dustbox::presets

