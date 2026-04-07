#include "PresetManager.h"

const juce::String PresetManager::presetFileExtension = ".phazon";
const int          PresetManager::numFactoryPresets   = 10;

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts)
    : apvts_ (apvts)
{}

//==============================================================================
// Helpers
//==============================================================================

void PresetManager::applyParam (const juce::String& id, float rawValue)
{
    if (auto* p = apvts_.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (rawValue));
}

juce::File PresetManager::getUserPresetsFolder() const
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                      .getChildFile ("Phazon")
                      .getChildFile ("Presets");

    if (!folder.isDirectory())
        folder.createDirectory();

    return folder;
}

//==============================================================================
// User presets
//==============================================================================

bool PresetManager::saveUserPreset (const juce::String& name)
{
    auto file = getUserPresetsFolder().getChildFile (name + presetFileExtension);

    auto state = apvts_.copyState();
    if (auto xml = state.createXml())
        return xml->writeTo (file);

    return false;
}

bool PresetManager::loadUserPreset (const juce::String& name)
{
    auto file = getUserPresetsFolder().getChildFile (name + presetFileExtension);

    if (!file.existsAsFile())
        return false;

    if (auto xml = juce::XmlDocument::parse (file))
    {
        if (xml->hasTagName (apvts_.state.getType()))
        {
            apvts_.replaceState (juce::ValueTree::fromXml (*xml));
            return true;
        }
    }

    return false;
}

bool PresetManager::deleteUserPreset (const juce::String& name)
{
    auto file = getUserPresetsFolder().getChildFile (name + presetFileExtension);
    return file.existsAsFile() && file.deleteFile();
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;

    for (auto& f : getUserPresetsFolder().findChildFiles (
             juce::File::findFiles, false, "*" + presetFileExtension))
    {
        names.add (f.getFileNameWithoutExtension());
    }

    names.sort (false);
    return names;
}

//==============================================================================
// Factory presets
//==============================================================================

juce::StringArray PresetManager::getFactoryPresetNames() const
{
    return { "Bowed String",
             "Struck Metal",
             "Membrane",
             "Drone",
             "Chaos",
             "Plucked",
             "Bass String",
             "Glass Harmonic",
             "Wood Block",
             "Shimmer Pad" };
}

void PresetManager::loadFactoryPreset (int index)
{
    switch (index)
    {
        case 0:  applyFactoryPreset0_BowedString();   break;
        case 1:  applyFactoryPreset1_StruckMetal();   break;
        case 2:  applyFactoryPreset2_Membrane();      break;
        case 3:  applyFactoryPreset3_Drone();         break;
        case 4:  applyFactoryPreset4_Chaos();         break;
        case 5:  applyFactoryPreset5_Plucked();       break;
        case 6:  applyFactoryPreset6_BassString();    break;
        case 7:  applyFactoryPreset7_GlassHarmonic(); break;
        case 8:  applyFactoryPreset8_WoodBlock();     break;
        case 9:  applyFactoryPreset9_ShimmerPad();    break;
        default: jassertfalse;                        break;
    }
}

//==============================================================================
// Factory preset definitions
// All values are in the raw (non-normalised) parameter units.
//==============================================================================

void PresetManager::applyFactoryPreset0_BowedString()
{
    applyParam ("springDamping",   0.20f);
    applyParam ("massDamping",     0.02f);
    applyParam ("bowForce",        0.50f);
    applyParam ("nonlinearity",    0.25f);
    applyParam ("excitationRatio", 0.30f);
    applyParam ("bowWidth",        0.05f);
    applyParam ("drive",           0.30f);
    applyParam ("dimensions",      8.0f);
    applyParam ("massProfile",     0.0f); // 1D
    applyParam ("octave",          0.0f);
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.005f);
    applyParam ("release",         0.300f);
    applyParam ("SpaceMix",        0.15f);
    applyParam ("SpaceSize",       0.40f);
    applyParam ("chaos",           0.0f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset1_StruckMetal()
{
    applyParam ("springDamping",   0.05f);
    applyParam ("massDamping",     0.01f);
    applyParam ("bowForce",        0.80f);
    applyParam ("nonlinearity",    0.10f);
    applyParam ("excitationRatio", 0.25f);
    applyParam ("bowWidth",        0.08f);
    applyParam ("drive",           0.50f);
    applyParam ("dimensions",      12.0f);
    applyParam ("massProfile",     0.0f); // 1D
    applyParam ("octave",          0.0f);
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.002f);
    applyParam ("release",         1.200f);
    applyParam ("SpaceMix",        0.25f);
    applyParam ("SpaceSize",       0.60f);
    applyParam ("chaos",           0.0f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset2_Membrane()
{
    applyParam ("springDamping",   0.15f);
    applyParam ("massDamping",     0.03f);
    applyParam ("bowForce",        0.60f);
    applyParam ("nonlinearity",    0.20f);
    applyParam ("excitationRatio", 0.35f);
    applyParam ("bowWidth",        0.10f);
    applyParam ("drive",           0.35f);
    applyParam ("dimensions",      16.0f);
    applyParam ("massProfile",     1.0f); // 2D
    applyParam ("octave",          0.0f);
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.010f);
    applyParam ("release",         0.600f);
    applyParam ("SpaceMix",        0.40f);
    applyParam ("SpaceSize",       0.55f);
    applyParam ("chaos",           0.0f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset3_Drone()
{
    applyParam ("springDamping",   0.30f);
    applyParam ("massDamping",     0.01f);
    applyParam ("bowForce",        0.40f);
    applyParam ("nonlinearity",    0.35f);
    applyParam ("excitationRatio", 0.15f);
    applyParam ("bowWidth",        0.04f);
    applyParam ("drive",           0.25f);
    applyParam ("dimensions",      8.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",         -12.0f); // one octave down
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.200f);
    applyParam ("release",         2.000f);
    applyParam ("MovementFREQ",    0.15f);
    applyParam ("MovementDEPTH",   0.08f);
    applyParam ("SpaceMix",        0.35f);
    applyParam ("SpaceSize",       0.70f);
    applyParam ("chaos",           0.05f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset4_Chaos()
{
    applyParam ("springDamping",   0.10f);
    applyParam ("massDamping",     0.02f);
    applyParam ("bowForce",        0.65f);
    applyParam ("nonlinearity",    0.70f);
    applyParam ("excitationRatio", 0.40f);
    applyParam ("bowWidth",        0.07f);
    applyParam ("drive",           0.60f);
    applyParam ("dimensions",      16.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",          0.0f);
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.005f);
    applyParam ("release",         0.500f);
    applyParam ("SpaceMix",        0.20f);
    applyParam ("SpaceSize",       0.50f);
    applyParam ("chaos",           0.60f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset5_Plucked()
{
    applyParam ("springDamping",   0.25f);
    applyParam ("massDamping",     0.05f);
    applyParam ("bowForce",        0.70f);
    applyParam ("nonlinearity",    0.20f);
    applyParam ("excitationRatio", 0.25f);
    applyParam ("bowWidth",        0.06f);
    applyParam ("drive",           0.40f);
    applyParam ("dimensions",      8.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",          0.0f);
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.002f);
    applyParam ("release",         0.500f);
    applyParam ("SpaceMix",        0.10f);
    applyParam ("SpaceSize",       0.35f);
    applyParam ("chaos",           0.0f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset6_BassString()
{
    applyParam ("springDamping",   0.35f);
    applyParam ("massDamping",     0.04f);
    applyParam ("bowForce",        0.55f);
    applyParam ("nonlinearity",    0.30f);
    applyParam ("excitationRatio", 0.20f);
    applyParam ("bowWidth",        0.05f);
    applyParam ("drive",           0.45f);
    applyParam ("dimensions",      12.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",         -12.0f); // one octave down
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.015f);
    applyParam ("release",         0.800f);
    applyParam ("SpaceMix",        0.10f);
    applyParam ("SpaceSize",       0.30f);
    applyParam ("chaos",           0.0f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset7_GlassHarmonic()
{
    applyParam ("springDamping",   0.08f);
    applyParam ("massDamping",     0.01f);
    applyParam ("bowForce",        0.45f);
    applyParam ("nonlinearity",    0.15f);
    applyParam ("excitationRatio", 0.35f);
    applyParam ("bowWidth",        0.04f);
    applyParam ("drive",           0.10f);
    applyParam ("dimensions",      8.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",         12.0f); // one octave up
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.080f);
    applyParam ("release",         1.500f);
    applyParam ("SpaceMix",        0.50f);
    applyParam ("SpaceSize",       0.70f);
    applyParam ("chaos",           0.0f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset8_WoodBlock()
{
    applyParam ("springDamping",   0.40f);
    applyParam ("massDamping",     0.06f);
    applyParam ("bowForce",        0.75f);
    applyParam ("nonlinearity",    0.30f);
    applyParam ("excitationRatio", 0.50f);
    applyParam ("bowWidth",        0.08f);
    applyParam ("drive",           0.40f);
    applyParam ("dimensions",      6.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",          0.0f);
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.001f);
    applyParam ("release",         0.200f);
    applyParam ("SpaceMix",        0.05f);
    applyParam ("SpaceSize",       0.20f);
    applyParam ("chaos",           0.0f);
    applyParam ("ecoMode",         0.0f);
}

void PresetManager::applyFactoryPreset9_ShimmerPad()
{
    applyParam ("springDamping",   0.12f);
    applyParam ("massDamping",     0.02f);
    applyParam ("bowForce",        0.35f);
    applyParam ("nonlinearity",    0.40f);
    applyParam ("excitationRatio", 0.30f);
    applyParam ("bowWidth",        0.05f);
    applyParam ("drive",           0.20f);
    applyParam ("dimensions",      8.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",         12.0f); // one octave up
    applyParam ("detune",          0.0f);
    applyParam ("attack",          0.500f);
    applyParam ("release",         3.000f);
    applyParam ("vibratoFreq",     0.30f);
    applyParam ("vibratoDepth",    0.15f);
    applyParam ("SpaceMix",        0.70f);
    applyParam ("SpaceSize",       0.80f);
    applyParam ("chaos",           0.05f);
    applyParam ("ecoMode",         0.0f);
}
