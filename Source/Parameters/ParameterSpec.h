/*
  ==============================================================================
  File: ParameterSpec.h
  Responsibility: Provide helper structures for describing parameter metadata.
  Assumptions: Used primarily to assist with APVTS layout construction.
  TODO: Extend with localisation/formatter hooks when UI matures.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace dustbox::params
{
struct FloatSpec
{
    juce::String id;
    juce::String name;
    float minValue;
    float maxValue;
    float defaultValue;
    float skew = 0.0f;
};

struct IntSpec
{
    juce::String id;
    juce::String name;
    int minValue;
    int maxValue;
    int defaultValue;
};

struct BoolSpec
{
    juce::String id;
    juce::String name;
    bool defaultValue;
};

struct ChoiceSpec
{
    juce::String id;
    juce::String name;
    juce::StringArray choices;
    int defaultIndex;
};
} // namespace dustbox::params

