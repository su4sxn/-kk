#include "PluginProcessor.h"
#include "PluginEditor.h"

AtmoGrainAudioProcessorEditor::AtmoGrainAudioProcessorEditor(AtmoGrainAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    addAndMakeVisible(loadButton);
    loadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select a sample...", juce::File(), "*.wav;*.aif;*.aiff;*.mp3;*.flac");

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
                loadFile(file);
        });
    };

    fileLabel.setText(processorRef.getLoadedFileName(), juce::dontSendNotification);
    fileLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fileLabel);

    struct ParamDef { const char* id; const char* name; };
    static const ParamDef defs[] = {
        { "grainSize", "Size" }, { "density", "Density" }, { "position", "Position" }, { "posSpread", "Jitter" },
        { "pitchSpread", "PitchSpr" }, { "width", "Width" }, { "smear", "Smear" }, { "mix", "Mix" }
    };

    for (auto& d : defs)
    {
        auto* slider = new juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        addAndMakeVisible(slider);
        sliders.add(slider);

        auto* label = new juce::Label({}, d.name);
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        sliderLabels.add(label);

        attachments.add(new juce::AudioProcessorValueTreeState::SliderAttachment(processorRef.apvts, d.id, *slider));
    }

    setSize(560, 320);
}

AtmoGrainAudioProcessorEditor::~AtmoGrainAudioProcessorEditor() {}

void AtmoGrainAudioProcessorEditor::loadFile(const juce::File& file)
{
    processorRef.loadSampleFile(file);
    fileLabel.setText(processorRef.getLoadedFileName(), juce::dontSendNotification);
}

bool AtmoGrainAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".aif")
            || f.endsWithIgnoreCase(".aiff") || f.endsWithIgnoreCase(".mp3") || f.endsWithIgnoreCase(".flac"))
            return true;
    return false;
}

void AtmoGrainAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.size() > 0)
        loadFile(juce::File(files[0]));
}

void AtmoGrainAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a22));
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("AtmoGrain", getLocalBounds().removeFromTop(34), juce::Justification::centred, 1);

    g.setColour(juce::Colours::grey);
    g.setFont(13.0f);
    g.drawFittedText("drag a sample in, or hit Load", getLocalBounds().removeFromBottom(20), juce::Justification::centred, 1);
}

void AtmoGrainAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(34); // title

    auto topRow = area.removeFromTop(30);
    loadButton.setBounds(topRow.removeFromLeft(140));
    topRow.removeFromLeft(10);
    fileLabel.setBounds(topRow);

    area.removeFromTop(10);
    area.removeFromBottom(20); // footer hint

    const int cols = 4;
    const int rows = 2;
    const int w = area.getWidth() / cols;
    const int h = area.getHeight() / rows;

    for (int i = 0; i < sliders.size(); ++i)
    {
        const int col = i % cols;
        const int row = i / cols;
        auto cell = juce::Rectangle<int>(area.getX() + col * w, area.getY() + row * h, w, h);
        sliderLabels[i]->setBounds(cell.removeFromTop(16));
        sliders[i]->setBounds(cell);
    }
}
