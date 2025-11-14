/*
  ==============================================================================
  File: ParameterLayout.h
  Responsibility: Construct the AudioProcessorValueTreeState layout for Dustbox.
  Assumptions: Layout is consumed by DustboxProcessor during construction.
  TODO: Add parameter metadata (tooltips, value formatters) once UI expands.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "ParameterIDs.h"
#include "ParameterSpec.h"

namespace dustbox::params
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
    Layout layout;

    auto makeFloat = [](const FloatSpec& spec)
    {
        auto range = juce::NormalisableRange<float> { spec.minValue, spec.maxValue };
        if (spec.skew > 0.0f)
            range.setSkewForCentre(spec.skew);

        auto param = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { spec.id, 1 },
            spec.name,
            range,
            spec.defaultValue);
        return param;
    };

    auto makeChoice = [](const ChoiceSpec& spec)
    {
        auto param = std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { spec.id, 1 },
            spec.name,
            spec.choices,
            spec.defaultIndex);
        return param;
    };

    auto makeInt = [](const IntSpec& spec)
    {
        auto param = std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { spec.id, 1 },
            spec.name,
            spec.minValue,
            spec.maxValue,
            spec.defaultValue);
        return param;
    };

    auto makeBool = [](const BoolSpec& spec)
    {
        auto param = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { spec.id, 1 },
            spec.name,
            spec.defaultValue);
        return param;
    };

    // Tape
    layout.add(makeFloat({ ids::tapeWowDepth, "Wow Depth", 0.0f, 1.0f, 0.15f }));
    layout.add(makeFloat({ ids::tapeWowRateHz, "Wow Rate", 0.10f, 5.0f, 0.60f }));
    layout.add(makeFloat({ ids::tapeFlutterDepth, "Flutter Depth", 0.0f, 1.0f, 0.08f }));

    auto toneRange = juce::NormalisableRange<float> { 2000.0f, 20000.0f };
    toneRange.setSkewForCentre(11000.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ids::tapeToneLowpassHz, 1 },
        "Tone Low-pass",
        toneRange,
        11000.0f));

    layout.add(makeFloat({ ids::tapeNoiseLevelDb, "Noise Level", -60.0f, -20.0f, -48.0f }));
    layout.add(makeChoice({ ids::noiseRouting,
                            "Noise Routing",
                            juce::StringArray { "pre_tape", "post_tape", "parallel" },
                            1 }));

    // Dirt
    layout.add(makeFloat({ ids::dirtSaturationAmt, "Saturation", 0.0f, 1.0f, 0.35f }));
    layout.add(makeInt({ ids::dirtBitDepthBits, "Bit Depth", 4, 24, 12 }));
    layout.add(makeInt({ ids::dirtSampleRateDiv, "Sample Rate Div", 1, 16, 2 }));

    // Reverb
    layout.add(makeFloat({ ids::reverbPreDelayMs, "Reverb Pre-delay", 0.0f, 120.0f, 20.0f, 30.0f }));
    layout.add(makeFloat({ ids::reverbDecayTime, "Reverb Decay", 0.10f, 8.0f, 1.8f, 1.0f }));
    layout.add(makeFloat({ ids::reverbDamping, "Reverb Damping", 0.0f, 1.0f, 0.35f }));
    layout.add(makeFloat({ ids::reverbMix, "Reverb Mix", 0.0f, 1.0f, 0.25f }));

    // Pump
    layout.add(makeFloat({ ids::pumpAmount, "Pump Amount", 0.0f, 1.0f, 0.35f }));
    layout.add(makeChoice({ ids::pumpSyncNote,
                            "Pump Sync Note",
                            juce::StringArray { "1/4", "1/8", "1/16" },
                            1 }));
    layout.add(makeFloat({ ids::pumpPhase, "Pump Phase", 0.0f, 1.0f, 0.0f }));

    // Global
    layout.add(makeFloat({ ids::mixWet, "Wet Mix", 0.0f, 1.0f, 0.5f }));
    layout.add(makeFloat({ ids::outputGainDb, "Output Gain", -24.0f, 24.0f, 0.0f }));
    layout.add(makeBool({ ids::hardBypass, "Hard Bypass", false }));

    return layout;
}
} // namespace dustbox::params

