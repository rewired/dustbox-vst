/*
  ==============================================================================
  File: NoiseGenerator.h
  Responsibility: Provide a lightweight deterministic noise source for hiss.
  Assumptions: Generator is advanced only from realtime audio threads.
  TODO: Consider higher-quality noise or shared generator when DSP finalised.
  ==============================================================================
*/

#pragma once

#include <cstdint>

namespace dustbox::dsp
{
class NoiseGenerator
{
public:
    void seed(uint32_t value) noexcept { state = value ? value : 0x12345678u; }

    float getNextSample() noexcept
    {
        // XorShift32 generator
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(static_cast<int32_t>(state)) / static_cast<float>(0x7fffffff);
    }

private:
    uint32_t state { 0x12345678u };
};
} // namespace dustbox::dsp

