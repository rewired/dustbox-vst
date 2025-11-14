#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cmath>

#include "Source/Parameters/ParameterLayout.h"
#include "Source/Presets/FactoryPresets.h"

namespace
{
class DummyProcessor : public juce::AudioProcessor
{
public:
    DummyProcessor()
        : juce::AudioProcessor(BusesProperties())
    {
    }

    const juce::String getName() const override { return "DummyProcessor"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}

    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

bool approximatelyEqual(float a, float b, float tolerance = 1.0e-3f)
{
    return std::abs(a - b) <= tolerance;
}

bool checkProperty(const juce::ValueTree& tree, const juce::Identifier& id, float expected)
{
    if (! tree.hasProperty(id))
    {
        juce::Logger::writeToLog("Missing property: " + id.toString());
        return false;
    }

    auto actual = static_cast<float>(tree.getProperty(id));
    if (! approximatelyEqual(actual, expected))
    {
        juce::Logger::writeToLog("Property mismatch for " + id.toString() +
                                 ": expected " + juce::String(expected) +
                                 ", got " + juce::String(actual));
        return false;
    }

    return true;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    DummyProcessor processor;
    auto layout = dustbox::params::createParameterLayout();
    juce::AudioProcessorValueTreeState apvts { processor, nullptr, "Parameters", std::move(layout) };

    const auto presets = dustbox::presets::createFactoryPresets(apvts);
    if (presets.size() != 5)
    {
        juce::Logger::writeToLog("Unexpected preset count: " + juce::String(static_cast<int>(presets.size())));
        return 1;
    }

    struct ExpectedReverb
    {
        const char* name;
        float preDelay;
        float decay;
        float damping;
        float mix;
    };

    const ExpectedReverb expectations[] {
        { "Subtle Glue", 18.0f, 0.90f, 0.60f, 0.18f },
        { "Lo-Fi Hiss", 32.0f, 1.60f, 0.52f, 0.32f },
        { "Chorus Pump", 24.0f, 2.40f, 0.45f, 0.40f },
        { "Warm Crunch", 12.0f, 1.20f, 0.68f, 0.22f },
        { "Noisy Parallel", 28.0f, 1.80f, 0.55f, 0.0f },
    };

    jassert(static_cast<size_t>(std::size(expectations)) == presets.size());

    for (size_t index = 0; index < presets.size(); ++index)
    {
        const auto& preset = presets[index];
        const auto& expected = expectations[index];

        if (preset.name != expected.name)
        {
            juce::Logger::writeToLog("Preset order mismatch: expected " + juce::String(expected.name) +
                                     ", got " + preset.name);
            return 1;
        }

        const auto& state = preset.state;
        const bool ok = checkProperty(state, dustbox::params::ids::reverbPreDelayMs, expected.preDelay)
                        && checkProperty(state, dustbox::params::ids::reverbDecayTime, expected.decay)
                        && checkProperty(state, dustbox::params::ids::reverbDamping, expected.damping)
                        && checkProperty(state, dustbox::params::ids::reverbMix, expected.mix);

        if (! ok)
            return 1;
    }

    juce::Logger::writeToLog("Preset ValueTree verification passed.");
    return 0;
}
