/*
  ==============================================================================
  File: ProcessingGraph.h
  Responsibility: Provide a simple linear processing graph abstraction for the
                  Dustbox signal chain.
  Assumptions: Graph order is fixed (Tape → Dirt → Pump) for MVP.
  TODO: Expand to support branching/parallel paths when feature scope increases.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace dustbox::dsp
{
class ProcessingGraph
{
public:
    template <typename ModuleA, typename ModuleB, typename ModuleC>
    void process(juce::AudioBuffer<float>& buffer,
                 int numSamples,
                 ModuleA& tape,
                 ModuleB& dirt,
                 ModuleC& pump)
    {
        tape.processBlock(buffer, numSamples);
        dirt.processBlock(buffer, numSamples);
        pump.processBlock(buffer, numSamples);
    }
};
} // namespace dustbox::dsp

