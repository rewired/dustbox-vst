/*
  ==============================================================================
  File: PumpModule.h
  Responsibility: Provide tempo-synchronised gain modulation for Dustbox.
  Assumptions: Host tempo info is refreshed once per block via setSync().
  Notes: Envelope shape is branch-light and suitable for realtime use.
  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>

namespace dustbox::dsp
{
class PumpModule
{
public:
    struct Parameters
    {
        float amount { 0.35f };
        int syncNoteIndex { 1 };
        float phaseOffset { 0.0f };
    };

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void setParameters(const Parameters& newParams) noexcept;
    void setSync(double samplesPerCycle, float phaseOffsetNormalised) noexcept;
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

private:
    static constexpr size_t maxSupportedChannels = 16;

    Parameters parameters {};

    double currentSampleRate { 44100.0 };
    int preparedBlockSize { 0 };
    int numChannelsPrepared { 0 };

    double samplesPerCycle { 44100.0 };
    double phaseIncrement { 1.0 / 44100.0 };
    double phase { 0.0 };
    float phaseOffset { 0.0f };
};
} // namespace dustbox::dsp

