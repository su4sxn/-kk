#pragma once
#include <JuceHeader.h>
#include "Grain.h"

class AtmoGrainAudioProcessor : public juce::AudioProcessor
{
public:
    AtmoGrainAudioProcessor();
    ~AtmoGrainAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AtmoGrain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void loadSampleFile(const juce::File& file);
    juce::String getLoadedFileName() const { return loadedFileName; }

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    GranularEngine granular;
    juce::AudioBuffer<float> granularBuffer;

    // simple stereo feedback delay used for the "smear" (atmosphere) stage
    juce::dsp::DelayLine<float> delayL { 96000 }, delayR { 96000 };
    float smearLpStateL = 0.0f, smearLpStateR = 0.0f;

    juce::String loadedFileName { "No sample loaded" };
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AtmoGrainAudioProcessor)
};
