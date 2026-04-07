#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout PhazonAudioProcessor::createParameterLayout()
{
    // Parameters will be added in subsequent tickets (CN-296+).
    return {};
}

PhazonAudioProcessor::PhazonAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout())
{
}

PhazonAudioProcessor::~PhazonAudioProcessor() {}

//==============================================================================
const juce::String PhazonAudioProcessor::getName() const { return JucePlugin_Name; }
bool PhazonAudioProcessor::acceptsMidi() const  { return true; }
bool PhazonAudioProcessor::producesMidi() const { return false; }
bool PhazonAudioProcessor::isMidiEffect() const { return false; }
double PhazonAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int PhazonAudioProcessor::getNumPrograms()                              { return 1; }
int PhazonAudioProcessor::getCurrentProgram()                           { return 0; }
void PhazonAudioProcessor::setCurrentProgram (int)                      {}
const juce::String PhazonAudioProcessor::getProgramName (int)           { return {}; }
void PhazonAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void PhazonAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    synthesiser_.setCurrentPlaybackSampleRate (sampleRate);
}

void PhazonAudioProcessor::releaseResources() {}

bool PhazonAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PhazonAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synthesiser_.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
juce::AudioProcessorEditor* PhazonAudioProcessor::createEditor()
{
    return new PhazonAudioProcessorEditor (*this);
}

bool PhazonAudioProcessor::hasEditor() const { return true; }

//==============================================================================
void PhazonAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PhazonAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhazonAudioProcessor();
}
