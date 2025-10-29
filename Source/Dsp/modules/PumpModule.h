/*
  ==============================================================================
  File: PumpModule.h
  Responsibility: Provide tempo-synchronised gain modulation for Dustbox.
  Assumptions: Host tempo information is supplied each block via setSync().
  TODO: Implement final envelope shape tuned for musical feel.
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
    void setSync(double samplesPerCycle, double phaseOffsetSamples) noexcept;
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

private:
    double currentSampleRate { 44100.0 };
    int preparedBlockSize { 0 };
    int numChannelsPrepared { 0 };

    Parameters parameters {};

    double samplesPerCycle { 44100.0 / 2.0 };
    double phaseOffset { 0.0 };
    double phase { 0.0 };
};
} // namespace dustbox::dsp

