/*
  ==============================================================================
  File: DenormalGuard.h
  Responsibility: Provide an RAII wrapper to disable denormals in critical
                  realtime sections.
  Assumptions: Guard instances are created on the audio thread.
  TODO: Profile the necessity once DSP code is finalised.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace dustbox::dsp
{
using DenormalGuard = juce::ScopedNoDenormals;
}

