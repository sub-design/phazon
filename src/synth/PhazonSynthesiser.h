#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "NetworkVoice.h"

/**
 * PhazonSynthesiser — MPE synthesiser with energy-based voice stealing.
 *
 * Default: 8 voices, 1-D physics profile.
 * ECO mode: 4 voices, each using NetworkIf::Profile::Eco (4-node solver).
 *
 * Voice stealing strategy: steal the currently-playing voice with the
 * lowest output RMS (lowest energy), favouring idle voices first.
 */
class PhazonSynthesiser  : public juce::MPESynthesiser
{
public:
    static constexpr int kDefaultPolyphony = 8;
    static constexpr int kEcoPolyphony     = 4;

    PhazonSynthesiser();

    /** Switch between full (8-voice, 8-node) and ECO (4-voice, 4-node) modes. */
    void setEcoMode (bool eco);

    /** Set the excitation mode for all voices. */
    void setExcitationMode (NetworkVoice::ExcitationMode mode);

    /** Push a base-params snapshot to all voices (called once per audio block). */
    void setBaseParams (const PhysicsParams& params);

protected:
    /**
     * Energy-based voice stealing.
     * Returns the first idle voice if one exists; otherwise returns the
     * currently-playing voice with the lowest output RMS.
     */
    juce::MPESynthesiserVoice* findVoiceToSteal (juce::MPENote noteToStealVoiceFor = juce::MPENote()) const override;

private:
    bool  ecoMode_        = false;
    int   activeVoiceCap_ = kDefaultPolyphony;
    NetworkVoice::ExcitationMode excitationMode_ = NetworkVoice::ExcitationMode::Bow;
};
