#include "PresetManager.h"

const juce::String PresetManager::presetFileExtension = ".phazon";
const int          PresetManager::numFactoryPresets   = 22;

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
             "Shimmer Pad",
             "Resin Cello",
             "Wire Viola",
             "Bent Bell",
             "Bronze Plate",
             "Skin Pulse",
             "Taut Frame Drum",
             "Dust Swarm",
             "Broken Orbit",
             "Harmonic Bloom",
             "Polar Drone",
             "Fog Choir",
             "Membrane Harp" };
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
        case 10: applyFactoryPreset10_ResinCello();   break;
        case 11: applyFactoryPreset11_WireViola();    break;
        case 12: applyFactoryPreset12_BentBell();     break;
        case 13: applyFactoryPreset13_BronzePlate();  break;
        case 14: applyFactoryPreset14_SkinPulse();    break;
        case 15: applyFactoryPreset15_TautFrameDrum(); break;
        case 16: applyFactoryPreset16_DustSwarm();    break;
        case 17: applyFactoryPreset17_BrokenOrbit();  break;
        case 18: applyFactoryPreset18_HarmonicBloom(); break;
        case 19: applyFactoryPreset19_PolarDrone();   break;
        case 20: applyFactoryPreset20_FogChoir();     break;
        case 21: applyFactoryPreset21_MembraneHarp(); break;
        default: jassertfalse;                        break;
    }
}

//==============================================================================
// Factory preset definitions
// All values are in the raw (non-normalised) parameter units.
//==============================================================================

void PresetManager::applyFactoryPreset0_BowedString()
{
    applyParam ("excitationMode",  0.0f);
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
    applyParam ("excitationMode",  2.0f);
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
    applyParam ("excitationMode",  2.0f);
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
    applyParam ("excitationMode",  0.0f);
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
    applyParam ("excitationMode",  0.0f);
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
    applyParam ("excitationMode",  1.0f);
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
    applyParam ("excitationMode",  0.0f);
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
    applyParam ("excitationMode",  1.0f);
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
    applyParam ("excitationMode",  2.0f);
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
    applyParam ("excitationMode",  0.0f);
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

void PresetManager::applyFactoryPreset10_ResinCello()
{
    applyParam ("excitationMode",  0.0f);
    applyParam ("springDamping",   0.24f);
    applyParam ("massDamping",     0.03f);
    applyParam ("bowForce",        0.58f);
    applyParam ("nonlinearity",    0.28f);
    applyParam ("excitationRatio", 0.21f);
    applyParam ("bowWidth",        0.06f);
    applyParam ("drive",           0.18f);
    applyParam ("dimensions",      10.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",         -12.0f);
    applyParam ("attack",          0.030f);
    applyParam ("release",         1.100f);
    applyParam ("SpaceMix",        0.18f);
    applyParam ("SpaceSize",       0.42f);
    applyParam ("filterCutoff",    8200.0f);
    applyParam ("filterResonance", 0.10f);
}

void PresetManager::applyFactoryPreset11_WireViola()
{
    applyParam ("excitationMode",  0.0f);
    applyParam ("springDamping",   0.12f);
    applyParam ("massDamping",     0.015f);
    applyParam ("bowForce",        0.47f);
    applyParam ("nonlinearity",    0.22f);
    applyParam ("excitationRatio", 0.27f);
    applyParam ("bowWidth",        0.04f);
    applyParam ("drive",           0.12f);
    applyParam ("dimensions",      12.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",          0.0f);
    applyParam ("attack",          0.012f);
    applyParam ("release",         0.900f);
    applyParam ("MovementFREQ",    0.24f);
    applyParam ("MovementDEPTH",   0.05f);
    applyParam ("SpaceMix",        0.22f);
    applyParam ("SpaceSize",       0.46f);
}

void PresetManager::applyFactoryPreset12_BentBell()
{
    applyParam ("excitationMode",  2.0f);
    applyParam ("springDamping",   0.06f);
    applyParam ("massDamping",     0.008f);
    applyParam ("bowForce",        0.85f);
    applyParam ("nonlinearity",    0.18f);
    applyParam ("excitationRatio", 0.33f);
    applyParam ("bowWidth",        0.09f);
    applyParam ("drive",           0.36f);
    applyParam ("dimensions",      14.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("attack",          0.001f);
    applyParam ("release",         1.700f);
    applyParam ("SpaceMix",        0.32f);
    applyParam ("SpaceSize",       0.64f);
    applyParam ("detune",         -0.08f);
}

void PresetManager::applyFactoryPreset13_BronzePlate()
{
    applyParam ("excitationMode",  2.0f);
    applyParam ("springDamping",   0.09f);
    applyParam ("massDamping",     0.016f);
    applyParam ("bowForce",        0.73f);
    applyParam ("nonlinearity",    0.26f);
    applyParam ("excitationRatio", 0.41f);
    applyParam ("bowWidth",        0.08f);
    applyParam ("drive",           0.28f);
    applyParam ("dimensions",      16.0f);
    applyParam ("massProfile",     1.0f);
    applyParam ("attack",          0.003f);
    applyParam ("release",         1.300f);
    applyParam ("SpaceMix",        0.28f);
    applyParam ("SpaceSize",       0.58f);
}

void PresetManager::applyFactoryPreset14_SkinPulse()
{
    applyParam ("excitationMode",  2.0f);
    applyParam ("springDamping",   0.26f);
    applyParam ("massDamping",     0.04f);
    applyParam ("bowForce",        0.69f);
    applyParam ("nonlinearity",    0.24f);
    applyParam ("excitationRatio", 0.46f);
    applyParam ("bowWidth",        0.10f);
    applyParam ("drive",           0.31f);
    applyParam ("dimensions",      9.0f);
    applyParam ("massProfile",     1.0f);
    applyParam ("attack",          0.001f);
    applyParam ("release",         0.420f);
    applyParam ("SpaceMix",        0.08f);
    applyParam ("SpaceSize",       0.24f);
}

void PresetManager::applyFactoryPreset15_TautFrameDrum()
{
    applyParam ("excitationMode",  1.0f);
    applyParam ("springDamping",   0.19f);
    applyParam ("massDamping",     0.028f);
    applyParam ("bowForce",        0.64f);
    applyParam ("nonlinearity",    0.20f);
    applyParam ("excitationRatio", 0.18f);
    applyParam ("bowWidth",        0.07f);
    applyParam ("drive",           0.22f);
    applyParam ("dimensions",      16.0f);
    applyParam ("massProfile",     1.0f);
    applyParam ("attack",          0.002f);
    applyParam ("release",         0.750f);
    applyParam ("SpaceMix",        0.14f);
    applyParam ("SpaceSize",       0.34f);
}

void PresetManager::applyFactoryPreset16_DustSwarm()
{
    applyParam ("excitationMode",  0.0f);
    applyParam ("springDamping",   0.08f);
    applyParam ("massDamping",     0.01f);
    applyParam ("bowForce",        0.62f);
    applyParam ("nonlinearity",    0.82f);
    applyParam ("excitationRatio", 0.52f);
    applyParam ("bowWidth",        0.11f);
    applyParam ("drive",           0.52f);
    applyParam ("dimensions",      18.0f);
    applyParam ("massProfile",     1.0f);
    applyParam ("attack",          0.015f);
    applyParam ("release",         1.600f);
    applyParam ("MovementFREQ",    0.41f);
    applyParam ("MovementDEPTH",   0.18f);
    applyParam ("chaos",           0.72f);
    applyParam ("SpaceMix",        0.45f);
    applyParam ("SpaceSize",       0.74f);
}

void PresetManager::applyFactoryPreset17_BrokenOrbit()
{
    applyParam ("excitationMode",  1.0f);
    applyParam ("springDamping",   0.10f);
    applyParam ("massDamping",     0.018f);
    applyParam ("bowForce",        0.55f);
    applyParam ("nonlinearity",    0.76f);
    applyParam ("excitationRatio", 0.13f);
    applyParam ("bowWidth",        0.05f);
    applyParam ("drive",           0.48f);
    applyParam ("dimensions",      12.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("attack",          0.006f);
    applyParam ("release",         1.900f);
    applyParam ("chaos",           0.58f);
    applyParam ("vibratoFreq",     0.12f);
    applyParam ("vibratoDepth",    0.22f);
    applyParam ("SpaceMix",        0.38f);
    applyParam ("SpaceSize",       0.68f);
}

void PresetManager::applyFactoryPreset18_HarmonicBloom()
{
    applyParam ("excitationMode",  1.0f);
    applyParam ("springDamping",   0.11f);
    applyParam ("massDamping",     0.014f);
    applyParam ("bowForce",        0.40f);
    applyParam ("nonlinearity",    0.30f);
    applyParam ("excitationRatio", 0.24f);
    applyParam ("bowWidth",        0.04f);
    applyParam ("drive",           0.14f);
    applyParam ("dimensions",      10.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",         12.0f);
    applyParam ("attack",          0.040f);
    applyParam ("release",         1.800f);
    applyParam ("SpaceMix",        0.48f);
    applyParam ("SpaceSize",       0.78f);
    applyParam ("filterCutoff",    9800.0f);
}

void PresetManager::applyFactoryPreset19_PolarDrone()
{
    applyParam ("excitationMode",  0.0f);
    applyParam ("springDamping",   0.28f);
    applyParam ("massDamping",     0.012f);
    applyParam ("bowForce",        0.33f);
    applyParam ("nonlinearity",    0.46f);
    applyParam ("excitationRatio", 0.11f);
    applyParam ("bowWidth",        0.03f);
    applyParam ("drive",           0.18f);
    applyParam ("dimensions",      8.0f);
    applyParam ("massProfile",     0.0f);
    applyParam ("octave",         -12.0f);
    applyParam ("attack",          0.400f);
    applyParam ("release",         4.500f);
    applyParam ("MovementFREQ",    0.08f);
    applyParam ("MovementDEPTH",   0.10f);
    applyParam ("SpaceMix",        0.52f);
    applyParam ("SpaceSize",       0.82f);
}

void PresetManager::applyFactoryPreset20_FogChoir()
{
    applyParam ("excitationMode",  0.0f);
    applyParam ("springDamping",   0.16f);
    applyParam ("massDamping",     0.02f);
    applyParam ("bowForce",        0.29f);
    applyParam ("nonlinearity",    0.44f);
    applyParam ("excitationRatio", 0.37f);
    applyParam ("bowWidth",        0.07f);
    applyParam ("drive",           0.12f);
    applyParam ("dimensions",      14.0f);
    applyParam ("massProfile",     1.0f);
    applyParam ("octave",          0.0f);
    applyParam ("attack",          0.650f);
    applyParam ("release",         5.000f);
    applyParam ("vibratoFreq",     0.18f);
    applyParam ("vibratoDepth",    0.08f);
    applyParam ("chaos",           0.16f);
    applyParam ("SpaceMix",        0.62f);
    applyParam ("SpaceSize",       0.88f);
}

void PresetManager::applyFactoryPreset21_MembraneHarp()
{
    applyParam ("excitationMode",  1.0f);
    applyParam ("springDamping",   0.14f);
    applyParam ("massDamping",     0.022f);
    applyParam ("bowForce",        0.48f);
    applyParam ("nonlinearity",    0.24f);
    applyParam ("excitationRatio", 0.31f);
    applyParam ("bowWidth",        0.06f);
    applyParam ("drive",           0.19f);
    applyParam ("dimensions",      16.0f);
    applyParam ("massProfile",     1.0f);
    applyParam ("attack",          0.010f);
    applyParam ("release",         1.050f);
    applyParam ("SpaceMix",        0.24f);
    applyParam ("SpaceSize",       0.50f);
    applyParam ("filterCutoff",    10800.0f);
}
