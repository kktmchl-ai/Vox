#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ChannelCore.h"

class VoxChainProcessor final : public juce::AudioProcessor
{
public:
    VoxChainProcessor();
    ~VoxChainProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // GUI telemetry (read-only, safe from the message thread)
    float inputMeterDb (int ch) const noexcept  { return ch < 2 ? inMeter[(size_t) ch].load() : -100.f; }
    float outputMeterDb (int ch) const noexcept { return ch < 2 ? outMeter[(size_t) ch].load() : -100.f; }
    float compGrDb() const noexcept  { return compGr.load(); }
    float deessGrDb() const noexcept { return deessGr.load(); }
    float limGrDb() const noexcept   { return limGr.load(); }
    double eqResponseDb (double freq) const { return core[0].eqRef().responseDb (freq); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pullParameters();

    voxchain::ChannelCore core[2];
    int preparedChannels = 2;

    std::atomic<float> inMeter[2] { -100.f, -100.f }, outMeter[2] { -100.f, -100.f };
    std::atomic<float> compGr { 0.f }, deessGr { 0.f }, limGr { 0.f };

    // cached raw parameter pointers
    std::atomic<float>* p (const char* id) { return apvts.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxChainProcessor)
};
