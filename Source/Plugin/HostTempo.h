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

#include <cmath>

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

            if (const auto ppq = position->getPpqPosition())
            {
                ppqPosition = *ppq;
                hasValidPpq = true;
            }
            else
            {
                hasValidPpq = false;
            }

            return;
        }

        resetToFallback();
    }

    double getBpm() const noexcept { return bpm; }

    bool hasHostPhase() const noexcept { return hasValidPpq; }

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

    void advanceFallbackPhase(int numSamples, double sampleRate, int noteIndex) noexcept
    {
        if (hasValidPpq)
            return;

        const auto samplesPerCycle = getSamplesPerCycle(sampleRate, noteIndex);
        if (samplesPerCycle <= 0.0)
            return;

        fallbackPhase += static_cast<double>(numSamples) / samplesPerCycle;
        fallbackPhase -= std::floor(fallbackPhase);
    }

    double getPhase01(int noteIndex) const noexcept
    {
        if (hasValidPpq)
        {
            const auto multiplier = noteIndex == 0 ? 1.0 : (noteIndex == 1 ? 2.0 : 4.0);
            auto cycles = ppqPosition * multiplier;
            return cycles - std::floor(cycles);
        }

        return fallbackPhase;
    }

private:
    void resetToFallback() noexcept
    {
        bpm = fallbackBpm;
        timeSigNumerator = 4;
        timeSigDenominator = 4;
        hasValidPpq = false;
    }

    double bpm { fallbackBpm };
    int timeSigNumerator { 4 };
    int timeSigDenominator { 4 };
    double ppqPosition { 0.0 };
    double fallbackPhase { 0.0 };
    bool hasValidPpq { false };

    static constexpr double fallbackBpm = 120.0;
};
} // namespace dustbox

