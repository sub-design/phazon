#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"
#include "synth/ModMatrix.h"

using namespace juce;

//==============================================================================
// Helpers
//==============================================================================

static std::unique_ptr<AudioParameterFloat> makeFloat (
    const char* id, const char* name,
    float min, float max, float defaultVal,
    float step = 0.001f)
{
    return std::make_unique<AudioParameterFloat> (
        ParameterID { id, 1 }, name,
        NormalisableRange<float> (min, max, step),
        defaultVal);
}

static std::unique_ptr<AudioParameterBool> makeBool (
    const char* id, const char* name, bool defaultVal)
{
    return std::make_unique<AudioParameterBool> (
        ParameterID { id, 1 }, name, defaultVal);
}

static std::unique_ptr<AudioParameterInt> makeInt (
    const char* id, const char* name,
    int min, int max, int defaultVal)
{
    return std::make_unique<AudioParameterInt> (
        ParameterID { id, 1 }, name, min, max, defaultVal);
}

static std::unique_ptr<AudioParameterChoice> makeChoice (
    const char* id, const char* name,
    StringArray choices, int defaultIndex)
{
    return std::make_unique<AudioParameterChoice> (
        ParameterID { id, 1 }, name, choices, defaultIndex);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PhazonAudioProcessor::createParameterLayout()
{
    using namespace ParamIDs;
    AudioProcessorValueTreeState::ParameterLayout layout;

    // ---- Physical model core ------------------------------------------------
    layout.add (makeFloat (springDamping,   "Spring Damping",   0.0f,  1.0f, 0.20f));
    layout.add (makeFloat (massDamping,     "Mass Damping",     0.0f,  1.0f, 0.02f));
    layout.add (makeFloat (bowForce,        "Bow Force",        0.0f,  1.0f, 0.50f));
    layout.add (makeFloat (nonlinearity,    "Nonlinearity",     0.0f,  1.0f, 0.25f));
    layout.add (makeFloat (excitationRatio,    "Excitation Ratio",    0.0f, 1.0f, 0.30f));
    layout.add (makeFloat (excitationLocation, "Excitation Location", 0.0f, 1.0f, 0.30f));
    layout.add (makeFloat (bowWidth,           "Bow Width",           0.0f, 1.0f, 0.05f));
    layout.add (makeFloat (drive,           "Drive",            0.0f,  1.0f, 0.30f));
    layout.add (makeInt   (dimensions,      "Dimensions",       2,     64,   8));
    layout.add (makeChoice (massProfile,   "Mass Profile",
        { "1D", "2D", "2D Anisotropic" }, 0));
    layout.add (makeChoice (springProfile, "Spring Profile",
        { "1D", "2D", "2D Anisotropic" }, 0));

    // ---- Automation wrappers — physical core --------------------------------
    static const StringArray lfoTypes   { "Sine", "Triangle", "Saw", "Square", "Random" };
    static const StringArray lfoModes   { "Free", "Retrigger", "Hold" };

    layout.add (makeFloat  (springDampingMIN,     "Spring Damping Min",     0.0f, 1.0f,  0.0f));
    layout.add (makeFloat  (springDampingMAX,     "Spring Damping Max",     0.0f, 1.0f,  1.0f));
    layout.add (makeFloat  (springDampingRATE,    "Spring Damping Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (springDampingTYPE,    "Spring Damping Type",    lfoTypes, 0));
    layout.add (makeBool   (springDampingSYNC,    "Spring Damping Sync",    false));
    layout.add (makeBool   (springDampingINV,     "Spring Damping Inv",     false));
    layout.add (makeChoice (springDampingLFOmode, "Spring Damping LFO Mode",lfoModes, 0));

    layout.add (makeFloat  (massDampingMIN,     "Mass Damping Min",     0.0f, 1.0f,  0.0f));
    layout.add (makeFloat  (massDampingMAX,     "Mass Damping Max",     0.0f, 1.0f,  1.0f));
    layout.add (makeFloat  (massDampingRATE,    "Mass Damping Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (massDampingTYPE,    "Mass Damping Type",    lfoTypes, 0));
    layout.add (makeBool   (massDampingSYNC,    "Mass Damping Sync",    false));
    layout.add (makeBool   (massDampingINV,     "Mass Damping Inv",     false));
    layout.add (makeChoice (massDampingLFOmode, "Mass Damping LFO Mode",lfoModes, 0));

    layout.add (makeFloat  (bowForceMIN,     "Bow Force Min",     0.0f, 1.0f,  0.0f));
    layout.add (makeFloat  (bowForceMAX,     "Bow Force Max",     0.0f, 1.0f,  1.0f));
    layout.add (makeFloat  (bowForceRATE,    "Bow Force Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (bowForceTYPE,    "Bow Force Type",    lfoTypes, 0));
    layout.add (makeBool   (bowForceSYNC,    "Bow Force Sync",    false));
    layout.add (makeBool   (bowForceINV,     "Bow Force Inv",     false));
    layout.add (makeChoice (bowForceLFOmode, "Bow Force LFO Mode",lfoModes, 0));

    layout.add (makeFloat  (nonlinearityMIN,     "Nonlinearity Min",     0.0f, 1.0f,  0.0f));
    layout.add (makeFloat  (nonlinearityMAX,     "Nonlinearity Max",     0.0f, 1.0f,  1.0f));
    layout.add (makeFloat  (nonlinearityRATE,    "Nonlinearity Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (nonlinearityTYPE,    "Nonlinearity Type",    lfoTypes, 0));
    layout.add (makeBool   (nonlinearitySYNC,    "Nonlinearity Sync",    false));
    layout.add (makeBool   (nonlinearityINV,     "Nonlinearity Inv",     false));
    layout.add (makeChoice (nonlinearityLFOmode, "Nonlinearity LFO Mode",lfoModes, 0));

    layout.add (makeFloat  (excitationRatioMIN,     "Excitation Ratio Min",     0.0f, 1.0f,  0.0f));
    layout.add (makeFloat  (excitationRatioMAX,     "Excitation Ratio Max",     0.0f, 1.0f,  1.0f));
    layout.add (makeFloat  (excitationRatioRATE,    "Excitation Ratio Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (excitationRatioTYPE,    "Excitation Ratio Type",    lfoTypes, 0));
    layout.add (makeBool   (excitationRatioSYNC,    "Excitation Ratio Sync",    false));
    layout.add (makeBool   (excitationRatioINV,     "Excitation Ratio Inv",     false));
    layout.add (makeChoice (excitationRatioLFOmode, "Excitation Ratio LFO Mode",lfoModes, 0));

    layout.add (makeFloat  (driveMIN,     "Drive Min",     0.0f, 1.0f,  0.0f));
    layout.add (makeFloat  (driveMAX,     "Drive Max",     0.0f, 1.0f,  1.0f));
    layout.add (makeFloat  (driveRATE,    "Drive Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (driveTYPE,    "Drive Type",    lfoTypes, 0));
    layout.add (makeBool   (driveSYNC,    "Drive Sync",    false));
    layout.add (makeBool   (driveINV,     "Drive Inv",     false));
    layout.add (makeChoice (driveLFOmode, "Drive LFO Mode",lfoModes, 0));

    // ---- Pitch / note -------------------------------------------------------
    layout.add (makeFloat (octave,  "Octave",   -24.0f, 24.0f, 0.0f, 1.0f));
    layout.add (makeFloat (detune,  "Detune",    -1.0f,  1.0f, 0.0f));
    layout.add (makeFloat (attack,  "Attack",    0.001f, 4.0f, 0.005f));
    layout.add (makeFloat (release, "Release",   0.010f, 8.0f, 0.300f));
    layout.add (makeBool  (monoMode,    "Mono Mode",    false));
    layout.add (makeBool  (releaseLock, "Release Lock", false));

    // ---- Movement LFO -------------------------------------------------------
    layout.add (makeFloat (MovementFREQ,  "Movement Freq",  0.01f, 20.0f, 0.50f));
    layout.add (makeFloat (MovementDEPTH, "Movement Depth", 0.0f,  1.0f,  0.0f));
    layout.add (makeBool  (MovementSYNC, "Movement Sync", false));
    layout.add (makeChoice (MovementTYPE, "Movement Type",
        { "Sine", "Triangle", "Saw", "Square", "Random" }, 0));
    layout.add (makeBool (MovementINV, "Movement Inv", false));
    layout.add (makeFloat (MovementMIN,     "Movement Min",     0.0f,  1.0f,  0.0f));
    layout.add (makeFloat (MovementMAX,     "Movement Max",     0.0f,  1.0f,  1.0f));
    layout.add (makeFloat (MovementRATE,    "Movement Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (MovementLFOmode, "Movement LFO Mode",
        { "Free", "Retrigger", "Hold" }, 0));

    // ---- Modulation LFO -----------------------------------------------------
    layout.add (makeFloat (ModulationFREQ,  "Modulation Freq",  0.01f, 20.0f, 0.50f));
    layout.add (makeFloat (ModulationDEPTH, "Modulation Depth", 0.0f,  1.0f,  0.0f));
    layout.add (makeBool  (ModulationSYNC, "Modulation Sync", false));
    layout.add (makeChoice (ModulationTYPE, "Modulation Type",
        { "Sine", "Triangle", "Saw", "Square", "Random" }, 0));
    layout.add (makeBool (ModulationINV, "Modulation Inv", false));
    layout.add (makeFloat (ModulationMIN,     "Modulation Min",     0.0f,  1.0f,  0.0f));
    layout.add (makeFloat (ModulationMAX,     "Modulation Max",     0.0f,  1.0f,  1.0f));
    layout.add (makeFloat (ModulationRATE,    "Modulation Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (ModulationLFOmode, "Modulation LFO Mode",
        { "Free", "Retrigger", "Hold" }, 0));

    // ---- Vibrato LFO --------------------------------------------------------
    layout.add (makeFloat (vibratoFreq,  "Vibrato Freq",  0.01f, 20.0f, 5.0f));
    layout.add (makeFloat (vibratoDepth, "Vibrato Depth", 0.0f,  1.0f,  0.0f));
    layout.add (makeBool  (vibratoSync, "Vibrato Sync", false));
    layout.add (makeChoice (VibratoTYPE, "Vibrato Type",
        { "Sine", "Triangle", "Saw", "Square", "Random" }, 0));
    layout.add (makeBool (VibratoINV, "Vibrato Inv", false));
    layout.add (makeFloat (VibratoMIN,     "Vibrato Min",     0.0f,  1.0f,  0.0f));
    layout.add (makeFloat (VibratoMAX,     "Vibrato Max",     0.0f,  1.0f,  1.0f));
    layout.add (makeFloat (VibratoRATE,    "Vibrato Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (VibratoLFOmode, "Vibrato LFO Mode",
        { "Free", "Retrigger", "Hold" }, 0));

    // ---- Filter -------------------------------------------------------------
    layout.add (makeFloat (filterCutoff,    "Filter Cutoff",    20.0f, 20000.0f, 20000.0f, 1.0f));
    layout.add (makeFloat (filterResonance, "Filter Resonance", 0.0f,  1.0f,    0.0f));
    layout.add (makeChoice (filterType, "Filter Type",
        { "LP", "HP", "BP", "Notch" }, 0));
    layout.add (makeFloat (filterMin,  "Filter Min",  0.0f, 1.0f, 0.0f));
    layout.add (makeFloat (filterMax,  "Filter Max",  0.0f, 1.0f, 1.0f));
    layout.add (makeBool  (filterInv,  "Filter Inv",  false));
    layout.add (makeBool  (filterLock, "Filter Lock", false));

    // ---- Space / reverb -----------------------------------------------------
    layout.add (makeFloat (SpaceMix,  "Space Mix",  0.0f, 1.0f, 0.15f));
    layout.add (makeFloat (SpaceSize, "Space Size", 0.0f, 1.0f, 0.40f));
    layout.add (makeFloat (SpaceMIN,     "Space Min",     0.0f,  1.0f,  0.0f));
    layout.add (makeFloat (SpaceMAX,     "Space Max",     0.0f,  1.0f,  1.0f));
    layout.add (makeFloat (SpaceRATE,    "Space Rate",    0.01f, 20.0f, 1.0f));
    layout.add (makeChoice (SpaceTYPE, "Space Type",
        { "Sine", "Triangle", "Saw", "Square", "Random" }, 0));
    layout.add (makeBool (SpaceSYNC, "Space Sync", false));
    layout.add (makeBool (SpaceINV,  "Space Inv",  false));
    layout.add (makeChoice (SpaceLFOmode, "Space LFO Mode",
        { "Free", "Retrigger", "Hold" }, 0));

    // ---- Output -------------------------------------------------------------
    layout.add (makeFloat (outputGain, "Output Gain", -24.0f, 12.0f, 0.0f, 0.1f));

    // ---- System / global ----------------------------------------------------
    layout.add (makeFloat (chaos,      "Chaos",     0.0f, 1.0f, 0.0f));
    layout.add (makeBool  (ecoMode,    "Eco Mode",  false));
    layout.add (makeChoice (excitationMode, "Excitation Mode",
        { "Bow", "Pluck", "Strike" }, 0));
    layout.add (makeBool  (mpeMode,    "MPE Mode",  true));
    layout.add (makeChoice (automationMode, "Automation Mode",
        { "Off", "LFO", "MPE", "Drift" }, 0));
    layout.add (makeFloat (driftAmount, "Drift Amount", 0.0f, 1.0f, 0.0f));

    // ---- Mod matrix ---------------------------------------------------------
    layout.add (makeFloat (modLfoRate,  "Mod LFO Rate",  0.01f, 20.0f, 1.0f));
    layout.add (makeFloat (modLfoDepth, "Mod LFO Depth", 0.0f,  1.0f,  0.0f));
    layout.add (makeChoice (modLfoShape, "Mod LFO Shape", { "Sine", "Square", "Random", "Triangle" }, 0));

    for (int i = 0; i < ModMatrix::kSlotCount; ++i)
    {
        layout.add (makeChoice (modSlotSource[(size_t) i], "Mod Slot Source",
                                { "Off", "LFO", "Pressure", "Timbre", "Velocity" }, 0));
        layout.add (makeChoice (modSlotDestination[(size_t) i], "Mod Slot Destination",
                                { "Off", "Chaos", "Force", "Overtones", "Cutoff", "Drive", "Body", "Spring" }, 0));
        layout.add (makeFloat (modSlotAmount[(size_t) i], "Mod Slot Amount", -1.0f, 1.0f, 0.0f));
    }

    return layout;
}

PhazonAudioProcessor::PhazonAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout()),
      presetManager (apvts)
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
void PhazonAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthesiser_.setCurrentPlaybackSampleRate (sampleRate);
    synthesiser_.prepare (samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 2;
    postProcess_.prepare (spec);
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

    // ---- Build PhysicsParams snapshot from APVTS atomics -------------------
    // getRawParameterValue returns a std::atomic<float>* — safe to read here.
    using namespace ParamIDs;
    auto get = [&] (const char* id) {
        return apvts.getRawParameterValue (id)->load (std::memory_order_relaxed);
    };

    PhysicsParams params;
    params.springDamping   = get (springDamping);
    params.massDamping     = get (massDamping);
    params.bowForce        = get (bowForce);
    params.nonlinearity    = get (nonlinearity);
    params.excitationRatio = get (excitationRatio);
    params.bowWidth        = get (bowWidth);
    params.drive           = get (drive);
    params.dimensions      = static_cast<int> (get (dimensions));
    params.massProfile     = static_cast<int> (get (massProfile));
    params.attack          = get (attack);
    params.release         = get (release);
    params.octave          = get (octave);
    params.detune          = get (detune);
    params.chaos           = get (chaos);

    synthesiser_.setBaseParams (params);
    ModMatrix::Config modMatrix;
    modMatrix.lfoRate = get (modLfoRate);
    modMatrix.lfoDepth = get (modLfoDepth);
    modMatrix.lfoShape = (int) get (modLfoShape);

    for (int i = 0; i < ModMatrix::kSlotCount; ++i)
    {
        auto& slot = modMatrix.slots[(size_t) i];
        slot.source = static_cast<ModMatrix::Source> ((int) get (modSlotSource[(size_t) i]));
        slot.destination = static_cast<ModMatrix::Destination> ((int) get (modSlotDestination[(size_t) i]));
        slot.amount = get (modSlotAmount[(size_t) i]);
    }

    synthesiser_.setModMatrixConfig (modMatrix);

    // ---- Eco mode -----------------------------------------------------------
    const bool eco = get (ecoMode) > 0.5f;
    synthesiser_.setEcoMode (eco);
    synthesiser_.setExcitationMode (static_cast<NetworkVoice::ExcitationMode> (
        juce::jlimit (0, 2, (int) get (excitationMode))));

    synthesiser_.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // ---- Post-processing chain (Drive → Filter → Space → Gain) -------------
    postProcess_.process (
        buffer,
        get (drive),
        get (filterCutoff),
        get (filterResonance),
        get (SpaceMix),
        get (SpaceSize),
        get (outputGain),
        synthesiser_.getDriveModulationBuffer().data(),
        synthesiser_.getCutoffModulationBuffer().data(),
        buffer.getNumSamples());
}

void PhazonAudioProcessor::getVisualizerData (float* nodes, int& count, float& rms) const noexcept
{
    synthesiser_.getVisualizerData (nodes, count, rms);
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
