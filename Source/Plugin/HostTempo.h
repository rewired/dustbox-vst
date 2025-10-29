/*
  ==============================================================================
  File: HostTempo.h
  Responsibility: Provide helper utilities to query and cache host tempo
                  information with sensible fallbacks.
  Assumptions: Owner updates the tempo each block using updateFromPlayHead().
  TODO: Support time signature awareness for future rhythmic features.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace dustbox
{
class HostTempo
{
public:
    void updateFromPlayHead(juce::AudioPlayHead* playHead)
    {
        if (playHead == nullptr)
        {
            resetToFallback();
            return;
        }

        if (const auto position = playHead->getPosition())
        {
            if (const auto bpmValue = position->getBpm())
                bpm = *bpmValue > 0.0 ? *bpmValue : fallbackBpm;
            else
                bpm = fallbackBpm;

            if (const auto signature = position->getTimeSignature())
            {
                timeSigNumerator = signature->numerator > 0 ? signature->numerator : 4;
                timeSigDenominator = signature->denominator > 0 ? signature->denominator : 4;
            }
            else
            {
                timeSigNumerator = 4;
                timeSigDenominator = 4;
            }

            return;
        }

        resetToFallback();
    }

    double getBpm() const noexcept { return bpm; }

    double getSamplesPerQuarterNote(double sampleRate) const noexcept
    {
        const auto secondsPerBeat = 60.0 / bpm;
        return secondsPerBeat * sampleRate;
    }

    double getSamplesForNoteValue(double sampleRate, int noteIndex) const noexcept
    {
        const auto quarterNoteSamples = getSamplesPerQuarterNote(sampleRate);
        switch (noteIndex)
        {
            case 0: return quarterNoteSamples;        // 1/4
            case 1: return quarterNoteSamples * 0.5;  // 1/8
            case 2: return quarterNoteSamples * 0.25; // 1/16
            default: return quarterNoteSamples * 0.5;
        }
    }

    double getSamplesPerCycle(double sampleRate, int noteIndex) const noexcept
    {
        return juce::jmax(1.0, getSamplesForNoteValue(sampleRate, noteIndex));
    }

private:
    void resetToFallback() noexcept
    {
        bpm = fallbackBpm;
        timeSigNumerator = 4;
        timeSigDenominator = 4;
    }

    double bpm { fallbackBpm };
    int timeSigNumerator { 4 };
    int timeSigDenominator { 4 };

    static constexpr double fallbackBpm = 120.0;
};
} // namespace dustbox

