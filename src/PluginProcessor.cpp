#include "PluginProcessor.h"
#include "PluginEditor.h"

// Parameter IDs from spec/parameters.md
static const juce::StringArray kParamIDs {
    "springDamping", "massDamping",
    "excitationRatio", "excitationLocation",
    "bowForce", "bowWidth",
    "nonlinearity", "drive",
    "dimensions",
    "MovementFREQ", "MovementDEPTH", "MovementSYNC",
    "ModulationFREQ", "ModulationDEPTH", "ModulationSYNC",
    "vibratoFreq", "vibratoDepth", "vibratoSync",
    "filterCutoff", "filterResonance",
    "SpaceMix", "SpaceSize",
    "octave", "detune",
    "attack", "release",
    "chaos", "ecoMode", "monoMode",
};

juce::AudioProcessorValueTreeState::ParameterLayout PhazonAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Physical model core
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "springDamping", "Spring Damping", 0.0f, 1.0f, 0.43f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "massDamping", "Mass Damping", 0.0f, 1.0f, 0.2f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "excitationRatio", "Excitation Ratio", 0.0f, 1.0f, 0.15f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "excitationLocation", "Excitation Location", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "bowForce", "Bow Force", 0.0f, 1.0f, 0.55f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "bowWidth", "Bow Width", 0.0f, 1.0f, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "nonlinearity", "Nonlinearity", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "drive", "Drive", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        "dimensions", "Dimensions", 4, 32, 8));

    // Movement (bow position LFO)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "MovementFREQ", "Movement Freq", 0.01f, 20.0f, 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "MovementDEPTH", "Movement Depth", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        "MovementSYNC", "Movement Sync", false));

    // Modulation (pickup position LFO)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "ModulationFREQ", "Modulation Freq", 0.01f, 20.0f, 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "ModulationDEPTH", "Modulation Depth", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        "ModulationSYNC", "Modulation Sync", false));

    // Vibrato (stiffness modulation)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "vibratoFreq", "Vibrato Freq", 0.01f, 20.0f, 5.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "vibratoDepth", "Vibrato Depth", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        "vibratoSync", "Vibrato Sync", false));

    // Filter
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "filterCutoff", "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f),
        20000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "filterResonance", "Filter Resonance", 0.0f, 1.0f, 0.0f));

    // Space / reverb
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "SpaceMix", "Space Mix", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "SpaceSize", "Space Size", 0.0f, 1.0f, 0.5f));

    // Pitch / note setup
    layout.add (std::make_unique<juce::AudioParameterInt> (
        "octave", "Octave", -3, 3, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "detune", "Detune", -100.0f, 100.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack", 0.001f, 2.0f, 0.01f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release", 0.01f, 5.0f, 0.3f));

    // Global modes
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "chaos", "Chaos", 0.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        "ecoMode", "Eco Mode", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        "monoMode", "Mono Mode", false));

    return layout;
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
double PhazonAudioProcessor::getTailLengthSeconds() const { return 2.0; }

int PhazonAudioProcessor::getNumPrograms()                              { return 1; }
int PhazonAudioProcessor::getCurrentProgram()                           { return 0; }
void PhazonAudioProcessor::setCurrentProgram (int)                      {}
const juce::String PhazonAudioProcessor::getProgramName (int)           { return {}; }
void PhazonAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void PhazonAudioProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/)
{
    // TODO CN-288: initialize BASynth::MPESynth and NetworkIf
}

void PhazonAudioProcessor::releaseResources() {}

bool PhazonAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PhazonAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    // TODO CN-288: route through MPESynth voice rendering
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
