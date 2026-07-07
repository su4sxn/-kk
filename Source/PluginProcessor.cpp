#include "PluginProcessor.h"
#include "PluginEditor.h"

AtmoGrainAudioProcessor::AtmoGrainAudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

AtmoGrainAudioProcessor::~AtmoGrainAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout AtmoGrainAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("grainSize", 1), "Grain Size",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel(" ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("density", 1), "Density",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f, 0.5f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel(" gr/s")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("position", 1), "Position", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("posSpread", 1), "Pos Spread", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("pitchSpread", 1), "Pitch Spread", 0.0f, 12.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Width", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("smear", 1), "Smear", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1), "Mix", 0.0f, 1.0f, 0.8f));

    return { params.begin(), params.end() };
}

void AtmoGrainAudioProcessor::loadSampleFile(const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return;

    juce::AudioBuffer<float> newBuffer((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read(&newBuffer, 0, (int) reader->lengthInSamples, 0, true, true);

    // sum down to mono for the grain engine
    juce::AudioBuffer<float> monoBuffer(1, newBuffer.getNumSamples());
    monoBuffer.clear();
    for (int ch = 0; ch < newBuffer.getNumChannels(); ++ch)
        monoBuffer.addFrom(0, 0, newBuffer, ch, 0, newBuffer.getNumSamples(), 1.0f / (float) newBuffer.getNumChannels());

    granular.setSample(std::move(monoBuffer));
    loadedFileName = file.getFileName();
}

void AtmoGrainAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    granular.prepare(sampleRate);
    granularBuffer.setSize(2, samplesPerBlock);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    delayL.prepare(spec);
    delayR.prepare(spec);
    delayL.setMaximumDelayInSamples(96000);
    delayR.setMaximumDelayInSamples(96000);
    smearLpStateL = smearLpStateR = 0.0f;
}

void AtmoGrainAudioProcessor::releaseResources() {}

bool AtmoGrainAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AtmoGrainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    granularBuffer.setSize(2, numSamples, false, false, true);
    granularBuffer.clear();

    granular.setParams(
        apvts.getRawParameterValue("grainSize")->load(),
        apvts.getRawParameterValue("density")->load(),
        apvts.getRawParameterValue("position")->load(),
        apvts.getRawParameterValue("posSpread")->load(),
        apvts.getRawParameterValue("pitchSpread")->load(),
        apvts.getRawParameterValue("width")->load());
    granular.process(granularBuffer);

    const float smear = apvts.getRawParameterValue("smear")->load();
    const float mix = apvts.getRawParameterValue("mix")->load();

    const float delayTimeMs = 120.0f + smear * 260.0f;
    const float feedback = smear * 0.75f;
    const float damp = 0.35f;
    const float delaySamples = (delayTimeMs * 0.001f) * (float) currentSampleRate;

    auto* gL = granularBuffer.getWritePointer(0);
    auto* gR = granularBuffer.getNumChannels() > 1 ? granularBuffer.getWritePointer(1) : gL;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dL = delayL.popSample(0, delaySamples);
        const float dR = delayR.popSample(0, delaySamples);

        smearLpStateL += damp * (dL - smearLpStateL);
        smearLpStateR += damp * (dR - smearLpStateR);

        const float inL = gL[i] + smearLpStateL * feedback;
        const float inR = gR[i] + smearLpStateR * feedback;

        delayL.pushSample(0, inL);
        delayR.pushSample(0, inR);

        gL[i] += smearLpStateL * smear;
        gR[i] += smearLpStateR * smear;
    }

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        auto* wet = granularBuffer.getReadPointer(juce::jmin(ch, granularBuffer.getNumChannels() - 1));
        for (int i = 0; i < numSamples; ++i)
            out[i] = out[i] * (1.0f - mix) + wet[i] * mix;
    }
}

void AtmoGrainAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AtmoGrainAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* AtmoGrainAudioProcessor::createEditor()
{
    return new AtmoGrainAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AtmoGrainAudioProcessor();
}
