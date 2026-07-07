#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AtmoGrainAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::FileDragAndDropTarget
{
public:
    explicit AtmoGrainAudioProcessorEditor(AtmoGrainAudioProcessor&);
    ~AtmoGrainAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void loadFile(const juce::File& file);

    AtmoGrainAudioProcessor& processorRef;

    juce::TextButton loadButton { "Load Sample..." };
    juce::Label fileLabel;

    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> sliderLabels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AtmoGrainAudioProcessorEditor)
};
