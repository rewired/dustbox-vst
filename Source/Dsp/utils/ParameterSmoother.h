/*
  ==============================================================================
  File: ParameterSmoother.h
  Responsibility: Provide lightweight wrappers around juce::SmoothedValue for
                  consistent smoothing behaviour across the processor.
  Assumptions: Smoothing is triggered from the audio thread only.
  TODO: Extend with exponential/logarithmic smoothing options per parameter.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace dustbox::dsp
{
class ParameterSmoother
{
public:
    void reset(double sampleRate, float timeMs)
    {
        smoothingTimeSeconds = timeMs * 0.001f;
        smoothed.reset(sampleRate, smoothingTimeSeconds);
    }

    void setTarget(float value) noexcept
    {
        smoothed.setTargetValue(value);
    }

    float getNextValue() noexcept
    {
        return smoothed.getNextValue();
    }

    void setImmediate(float value) noexcept
    {
        smoothed.setCurrentAndTargetValue(value);
    }

    float getCurrentValue() const noexcept
    {
        return smoothed.getCurrentValue();
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothed { 0.0f };
    float smoothingTimeSeconds { 0.02f };
};
} // namespace dustbox::dsp

