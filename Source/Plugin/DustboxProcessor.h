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
#include "../Dsp/modules/NoiseModule.h"
#include "../Dsp/modules/ReverbModule.h"
#include "../Dsp/modules/PumpModule.h"
#include "../Dsp/modules/TapeModule.h"
#include "../Dsp/utils/ParameterSmoother.h"
#include "../Presets/FactoryPresets.h"
#include "HostTempo.h"
#include "../Dsp/utils/DenormalGuard.h"

#include <array>
#include <atomic>
#include <vector>

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
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return valueTreeState; }
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept { return valueTreeState; }

    const HostTempo& getHostTempo() const noexcept { return hostTempo; }

    size_t getMeterChannelCount() const noexcept;
    float getInputPeakLevel(size_t channel) const noexcept;
    float getInputRmsLevel(size_t channel) const noexcept;
    bool getInputClipFlag(size_t channel) const noexcept;
    float getOutputPeakLevel(size_t channel) const noexcept;
    float getOutputRmsLevel(size_t channel) const noexcept;
    bool getOutputClipFlag(size_t channel) const noexcept;

private:
    struct MeterReadings
    {
        std::atomic<float> peak { 0.0f };
        std::atomic<float> rms { 0.0f };
        std::atomic<bool> clip { false };
    };

    static constexpr size_t meterChannelCount = 2;

    void updateParameters();
    void applyBypassRamp(juce::AudioBuffer<float>& buffer, int numSamples);
    void initialiseFactoryPresets();
    int findPresetIndexMatchingState(const juce::ValueTree& state) const;
    void publishMeterReadings(const juce::AudioBuffer<float>& buffer,
                              std::array<MeterReadings, meterChannelCount>& storage,
                              int numChannels,
                              int numSamples);

    juce::AudioProcessorValueTreeState valueTreeState;

    dsp::TapeModule tapeModule;
    dsp::NoiseModule noiseModule;
    dsp::DirtModule dirtModule;
    dsp::ReverbModule reverbModule;
    dsp::PumpModule pumpModule;
    dsp::ParameterSmoother wetMixSmoother;
    dsp::ParameterSmoother outputGainSmoother;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassSmoother;

    juce::AudioBuffer<float> dryBuffer;

    HostTempo hostTempo;

    double currentSampleRate { 44100.0 };
    int currentBlockSize { 0 };

    std::vector<presets::FactoryPreset> factoryPresets;
    int currentProgramIndex { 0 };

    std::array<MeterReadings, meterChannelCount> inputMeterValues {};
    std::array<MeterReadings, meterChannelCount> outputMeterValues {};

    struct CachedParameters
    {
        dsp::TapeModule::Parameters tapeParams;
        dsp::DirtModule::Parameters dirtParams;
        dsp::PumpModule::Parameters pumpParams;
        dsp::ReverbModule::Parameters reverbParams;
        dsp::NoiseModule::Parameters noiseParams;
        int noiseRoutingIndex { 1 };
        float reverbMix { 0.25f };
        float wetMix { 0.5f };
        float outputGain { 1.0f };
        bool hardBypass { false };
    } cachedParameters;

    bool bypassTransitionActive { false };
};
} // namespace dustbox

