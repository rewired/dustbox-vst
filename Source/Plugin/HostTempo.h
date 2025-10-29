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
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (playHead != nullptr && playHead->getCurrentPosition(info))
        {
            bpm = info.bpm > 0.0 ? info.bpm : fallbackBpm;
            timeSigNumerator = info.timeSigNumerator > 0 ? info.timeSigNumerator : 4;
            timeSigDenominator = info.timeSigDenominator > 0 ? info.timeSigDenominator : 4;
        }
        else
        {
            bpm = fallbackBpm;
            timeSigNumerator = 4;
            timeSigDenominator = 4;
        }
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

private:
    double bpm { fallbackBpm };
    int timeSigNumerator { 4 };
    int timeSigDenominator { 4 };

    static constexpr double fallbackBpm = 120.0;
};
} // namespace dustbox

