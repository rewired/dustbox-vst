/*
  ==============================================================================
  File: ParameterIDs.h
  Responsibility: Define stable parameter identifier strings for the APVTS.
  Assumptions: IDs remain constant for preset/automation compatibility.
  TODO: Review naming conventions before public release.
  ==============================================================================
*/

#pragma once

namespace dustbox::params
{
namespace ids
{
// Tape / Age
inline constexpr auto tapeWowDepth       = "tapeWowDepth";
inline constexpr auto tapeWowRateHz      = "tapeWowRateHz";
inline constexpr auto tapeFlutterDepth   = "tapeFlutterDepth";
inline constexpr auto tapeToneLowpassHz  = "tapeToneLowpassHz";
inline constexpr auto tapeNoiseLevelDb   = "tapeNoiseLevelDb";

// Noise
inline constexpr auto noiseRouting       = "noiseRouting";

// Dirt
inline constexpr auto dirtSaturationAmt  = "dirtSaturationAmt";
inline constexpr auto dirtBitDepthBits   = "dirtBitDepthBits";
inline constexpr auto dirtSampleRateDiv  = "dirtSampleRateDiv";

// Reverb
inline constexpr auto reverbPreDelayMs   = "reverbPreDelayMs";
inline constexpr auto reverbDecayTime    = "reverbDecayTime";
inline constexpr auto reverbDamping      = "reverbDamping";
inline constexpr auto reverbMix          = "reverbMix";

// Pump
inline constexpr auto pumpAmount         = "pumpAmount";
inline constexpr auto pumpSyncNote       = "pumpSyncNote";
inline constexpr auto pumpPhase          = "pumpPhase";

// Global
inline constexpr auto mixWet             = "mixWet";
inline constexpr auto outputGainDb       = "outputGainDb";
inline constexpr auto hardBypass         = "hardBypass";
} // namespace ids
} // namespace dustbox::params

