/*
  ==============================================================================
  File: DustboxProcessor.h
  Responsibility: Declare the Dustbox AudioProcessor, owning modules, parameters,
                  smoothing, and host tempo helpers.
  Assumptions: AudioProcessorValueTreeState manages parameter lifetime.
  TODO: Integrate advanced DSP once scaffolding validated.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "../Core/BuildConfig.h"
#include "../Core/Version.h"
#include "../Dsp/modules/DirtModule.h"
#include "../Dsp/modules/PumpModule.h"
#include "../Dsp/modules/TapeModule.h"
#include "../Dsp/utils/ParameterSmoother.h"
#include "HostTempo.h"
#include "../Dsp/utils/DenormalGuard.h"

namespace dustbox
{
class DustboxEditor;

class DustboxProcessor : public juce::AudioProcessor
{
public:
    DustboxProcessor();
    ~DustboxProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return dustbox::Version::projectName; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return valueTreeState; }

private:
    void updateParameters();
    void applyBypassRamp(juce::AudioBuffer<float>& buffer, int numSamples);
    void addNoisePostPump(juce::AudioBuffer<float>& wetBuffer, int numSamples);
    void addNoisePostMix(juce::AudioBuffer<float>& mixBuffer, int numSamples);

    juce::AudioProcessorValueTreeState valueTreeState;

    dsp::TapeModule tapeModule;
    dsp::DirtModule dirtModule;
    dsp::PumpModule pumpModule;
    dsp::ParameterSmoother wetMixSmoother;
    dsp::ParameterSmoother outputGainSmoother;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassSmoother;

    juce::AudioBuffer<float> dryBuffer;

    HostTempo hostTempo;

    double currentSampleRate { 44100.0 };
    int currentBlockSize { 0 };

    struct CachedParameters
    {
        dsp::TapeModule::Parameters tapeParams;
        dsp::DirtModule::Parameters dirtParams;
        dsp::PumpModule::Parameters pumpParams;
        float wetMix { 0.5f };
        float outputGain { 1.0f };
        bool hardBypass { false };
    } cachedParameters;

    bool bypassTransitionActive { false };
};
} // namespace dustbox

