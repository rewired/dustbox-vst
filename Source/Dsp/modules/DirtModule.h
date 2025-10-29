/*
  ==============================================================================
  File: DirtModule.h
  Responsibility: Encapsulate degradation processing (saturation/bit-depth/
                  downsampling) for the Dustbox signal chain.
  Assumptions: Module operates in-place on the provided buffer.
  TODO: Implement actual saturation curves and quantisation math.
  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace dustbox::dsp
{
class DirtModule
{
public:
    struct Parameters
    {
        float saturationAmount { 0.35f };
        int bitDepth { 12 };
        int sampleRateDiv { 2 };
    };

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();
    void setParameters(const Parameters& newParams) noexcept;
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

private:
    Parameters parameters {};
    double currentSampleRate { 44100.0 };
    int preparedBlockSize { 0 };
    int numChannelsPrepared { 0 };
    std::vector<int> downsampleCounters;
    std::vector<float> heldSamples;
};
} // namespace dustbox::dsp

