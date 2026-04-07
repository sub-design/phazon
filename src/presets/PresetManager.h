#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * PresetManager — save / load / delete user presets and factory presets.
 *
 * Preset format: .phazon files containing JUCE ValueTree XML matching the
 * APVTS state structure.
 *
 * User presets are stored in the platform application-data folder:
 *   macOS:   ~/Library/Application Support/Phazon/Presets/
 *   Windows: %APPDATA%\Phazon\Presets\
 *
 * Factory presets are built in-memory and never written to disk.
 */
class PresetManager
{
public:
    static const juce::String presetFileExtension; // ".phazon"
    static const int          numFactoryPresets;   // 22

    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);

    //==========================================================================
    // User presets

    /** Save the current APVTS state to <name>.phazon in the user presets folder.
     *  Overwrites any existing file with that name.
     *  @return true on success. */
    bool saveUserPreset   (const juce::String& name);

    /** Load a user preset by name (without extension).
     *  @return true if the file was found and loaded. */
    bool loadUserPreset   (const juce::String& name);

    /** Delete a user preset file.
     *  @return true if the file was deleted. */
    bool deleteUserPreset (const juce::String& name);

    /** Returns the names of all saved user presets (no extension, sorted). */
    juce::StringArray getUserPresetNames() const;

    /** Returns the platform-specific user presets folder (created on first call). */
    juce::File getUserPresetsFolder() const;

    //==========================================================================
    // Factory presets

    /** Returns the display names of all factory presets. */
    juce::StringArray getFactoryPresetNames() const;

    /** Load a factory preset by index [0, numFactoryPresets). */
    void loadFactoryPreset (int index);

private:
    juce::AudioProcessorValueTreeState& apvts_;

    /** Apply a raw parameter value using the normalised range of the parameter. */
    void applyParam (const juce::String& id, float rawValue);

    /** Defines the parameter values for each factory preset. */
    void applyFactoryPreset0_BowedString();
    void applyFactoryPreset1_StruckMetal();
    void applyFactoryPreset2_Membrane();
    void applyFactoryPreset3_Drone();
    void applyFactoryPreset4_Chaos();
    void applyFactoryPreset5_Plucked();
    void applyFactoryPreset6_BassString();
    void applyFactoryPreset7_GlassHarmonic();
    void applyFactoryPreset8_WoodBlock();
    void applyFactoryPreset9_ShimmerPad();
    void applyFactoryPreset10_ResinCello();
    void applyFactoryPreset11_WireViola();
    void applyFactoryPreset12_BentBell();
    void applyFactoryPreset13_BronzePlate();
    void applyFactoryPreset14_SkinPulse();
    void applyFactoryPreset15_TautFrameDrum();
    void applyFactoryPreset16_DustSwarm();
    void applyFactoryPreset17_BrokenOrbit();
    void applyFactoryPreset18_HarmonicBloom();
    void applyFactoryPreset19_PolarDrone();
    void applyFactoryPreset20_FogChoir();
    void applyFactoryPreset21_MembraneHarp();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
