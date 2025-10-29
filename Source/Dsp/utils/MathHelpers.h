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

struct EqualPowerGains
{
    float dry { 1.0f };
    float wet { 0.0f };
};

inline EqualPowerGains equalPowerMixGains(float mix) noexcept
{
    const auto clamped = juce::jlimit(0.0f, 1.0f, mix);
    const auto dry = std::cos(clamped * juce::MathConstants<float>::halfPi);
    const auto wet = std::sin(clamped * juce::MathConstants<float>::halfPi);
    return { dry, wet };
}

inline float softClip(float x) noexcept
{
    const auto x3 = x * x * x;
    return x - (x3 * 0.3333333333f);
}
}

