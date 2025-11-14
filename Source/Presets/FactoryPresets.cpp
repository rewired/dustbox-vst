/*
  ==============================================================================
  File: FactoryPresets.cpp
  Responsibility: Define Dustbox's host-visible factory presets as deterministic
                  AudioProcessorValueTreeState snapshots.
  Assumptions: Parameter IDs originate from ParameterIDs.h and remain stable.
  Notes: Every preset populates all parameter properties to ensure preset
         recalls match project state serialisation byte-for-byte.
  ==============================================================================
*/

#include "FactoryPresets.h"

#include "../Parameters/ParameterIDs.h"

namespace dustbox::presets
{
namespace
{
ParameterSetting makeSetting(const char* paramID, float value)
{
    return { juce::Identifier { paramID }, juce::var { value } };
}

ParameterSetting makeSetting(const char* paramID, double value)
{
    return { juce::Identifier { paramID }, juce::var { value } };
}

ParameterSetting makeSetting(const char* paramID, int value)
{
    return { juce::Identifier { paramID }, juce::var { value } };
}

ParameterSetting makeSetting(const char* paramID, bool value)
{
    return { juce::Identifier { paramID }, juce::var { value } };
}
}

juce::ValueTree makeStateFromMap(const juce::AudioProcessorValueTreeState& apvts,
                                 std::initializer_list<ParameterSetting> settings)
{
    auto state = apvts.state.createCopy();

    for (const auto& [id, value] : settings)
        state.setProperty(id, value, nullptr);

    return state;
}

std::vector<FactoryPreset> createFactoryPresets(const juce::AudioProcessorValueTreeState& apvts)
{
    using namespace params::ids;

    std::vector<FactoryPreset> presets;
    presets.reserve(5);

    presets.push_back({ "Subtle Glue",
                        makeStateFromMap(apvts,
                                         {
                                             makeSetting(tapeWowDepth, 0.10f),
                                             makeSetting(tapeWowRateHz, 0.55f),
                                             makeSetting(tapeFlutterDepth, 0.04f),
                                             makeSetting(tapeToneLowpassHz, 16000.0f),
                                             makeSetting(tapeNoiseLevelDb, -58.0f),
                                             makeSetting(noiseRouting, 1),
                                             makeSetting(dirtSaturationAmt, 0.12f),
                                             makeSetting(dirtBitDepthBits, 24),
                                             makeSetting(dirtSampleRateDiv, 1),
                                             makeSetting(pumpAmount, 0.08f),
                                             makeSetting(pumpSyncNote, 1),
                                             makeSetting(pumpPhase, 0.0f),
                                             makeSetting(reverbPreDelayMs, 18.0f),
                                             makeSetting(reverbDecayTime, 0.90f),
                                             makeSetting(reverbDamping, 0.60f),
                                             makeSetting(reverbMix, 0.18f),
                                             makeSetting(mixWet, 0.35f),
                                             makeSetting(outputGainDb, 0.0f),
                                             makeSetting(hardBypass, false),
                                         }) });

    presets.push_back({ "Lo-Fi Hiss",
                        makeStateFromMap(apvts,
                                         {
                                             makeSetting(tapeWowDepth, 0.22f),
                                             makeSetting(tapeWowRateHz, 0.65f),
                                             makeSetting(tapeFlutterDepth, 0.12f),
                                             makeSetting(tapeToneLowpassHz, 7800.0f),
                                             makeSetting(tapeNoiseLevelDb, -32.0f),
                                             makeSetting(noiseRouting, 1),
                                             makeSetting(dirtSaturationAmt, 0.28f),
                                             makeSetting(dirtBitDepthBits, 14),
                                             makeSetting(dirtSampleRateDiv, 3),
                                             makeSetting(pumpAmount, 0.15f),
                                             makeSetting(pumpSyncNote, 1),
                                             makeSetting(pumpPhase, 0.0f),
                                             makeSetting(reverbPreDelayMs, 32.0f),
                                             makeSetting(reverbDecayTime, 1.60f),
                                             makeSetting(reverbDamping, 0.52f),
                                             makeSetting(reverbMix, 0.32f),
                                             makeSetting(mixWet, 0.58f),
                                             makeSetting(outputGainDb, -0.5f),
                                             makeSetting(hardBypass, false),
                                         }) });

    presets.push_back({ "Chorus Pump",
                        makeStateFromMap(apvts,
                                         {
                                             makeSetting(tapeWowDepth, 0.35f),
                                             makeSetting(tapeWowRateHz, 1.50f),
                                             makeSetting(tapeFlutterDepth, 0.24f),
                                             makeSetting(tapeToneLowpassHz, 12500.0f),
                                             makeSetting(tapeNoiseLevelDb, -46.0f),
                                             makeSetting(noiseRouting, 2),
                                             makeSetting(dirtSaturationAmt, 0.10f),
                                             makeSetting(dirtBitDepthBits, 24),
                                             makeSetting(dirtSampleRateDiv, 1),
                                             makeSetting(pumpAmount, 0.65f),
                                             makeSetting(pumpSyncNote, 0),
                                             makeSetting(pumpPhase, 0.0f),
                                             makeSetting(reverbPreDelayMs, 24.0f),
                                             makeSetting(reverbDecayTime, 2.40f),
                                             makeSetting(reverbDamping, 0.45f),
                                             makeSetting(reverbMix, 0.40f),
                                             makeSetting(mixWet, 0.65f),
                                             makeSetting(outputGainDb, -0.5f),
                                             makeSetting(hardBypass, false),
                                         }) });

    presets.push_back({ "Warm Crunch",
                        makeStateFromMap(apvts,
                                         {
                                             makeSetting(tapeWowDepth, 0.24f),
                                             makeSetting(tapeWowRateHz, 0.75f),
                                             makeSetting(tapeFlutterDepth, 0.14f),
                                             makeSetting(tapeToneLowpassHz, 9000.0f),
                                             makeSetting(tapeNoiseLevelDb, -60.0f),
                                             makeSetting(noiseRouting, 0),
                                             makeSetting(dirtSaturationAmt, 0.52f),
                                             makeSetting(dirtBitDepthBits, 12),
                                             makeSetting(dirtSampleRateDiv, 2),
                                             makeSetting(pumpAmount, 0.18f),
                                             makeSetting(pumpSyncNote, 1),
                                             makeSetting(pumpPhase, 0.0f),
                                             makeSetting(reverbPreDelayMs, 12.0f),
                                             makeSetting(reverbDecayTime, 1.20f),
                                             makeSetting(reverbDamping, 0.68f),
                                             makeSetting(reverbMix, 0.22f),
                                             makeSetting(mixWet, 0.62f),
                                             makeSetting(outputGainDb, -1.0f),
                                             makeSetting(hardBypass, false),
                                         }) });

    presets.push_back({ "Noisy Parallel",
                        makeStateFromMap(apvts,
                                         {
                                             makeSetting(tapeWowDepth, 0.16f),
                                             makeSetting(tapeWowRateHz, 0.60f),
                                             makeSetting(tapeFlutterDepth, 0.08f),
                                             makeSetting(tapeToneLowpassHz, 11500.0f),
                                             makeSetting(tapeNoiseLevelDb, -30.0f),
                                             makeSetting(noiseRouting, 2),
                                             makeSetting(dirtSaturationAmt, 0.20f),
                                             makeSetting(dirtBitDepthBits, 18),
                                             makeSetting(dirtSampleRateDiv, 2),
                                             makeSetting(pumpAmount, 0.22f),
                                             makeSetting(pumpSyncNote, 1),
                                             makeSetting(pumpPhase, 0.25f),
                                             makeSetting(reverbPreDelayMs, 28.0f),
                                             makeSetting(reverbDecayTime, 1.80f),
                                             makeSetting(reverbDamping, 0.55f),
                                             makeSetting(reverbMix, 0.0f),
                                             makeSetting(mixWet, 0.45f),
                                             makeSetting(outputGainDb, -0.5f),
                                             makeSetting(hardBypass, false),
                                         }) });

    return presets;
}

} // namespace dustbox::presets

