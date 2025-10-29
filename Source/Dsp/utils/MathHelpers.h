/*
  ==============================================================================
  File: MathHelpers.h
  Responsibility: Provide utility math functions shared across DSP modules.
  Assumptions: Functions are simple inline helpers safe for realtime use.
  TODO: Expand with interpolation helpers when modulation shapes are defined.
  ==============================================================================
*/

#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace dustbox::dsp
{
inline float dbToGain(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

inline float clamp(float value, float minValue, float maxValue) noexcept
{
    return std::fmax(minValue, std::fmin(value, maxValue));
}

inline void applyEqualPowerMix(float wetAmount, float* drySample, float* wetSample) noexcept
{
    const auto dryGain = std::cos(wetAmount * juce::MathConstants<float>::halfPi);
    const auto wetGain = std::sin(wetAmount * juce::MathConstants<float>::halfPi);
    const auto dry = *drySample;
    const auto wet = *wetSample;
    *drySample = dry * dryGain + wet * wetGain;
}
}

