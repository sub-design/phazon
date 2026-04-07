#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/NetworkIf.h"

/**
 * NetworkVoice — one MPE synthesiser voice.
 *
 * Wraps NetworkIf (the physical solver) inside a juce::MPESynthesiserVoice.
 * Does NOT compute physics directly — delegates to NetworkIf::renderNextBlock.
 *
 * Signal path per block:
 *   NetworkIf::renderNextBlock()  →  ADSR  →  gain smoother  →  output buffer
 *
 * Lifecycle:
 *   noteStarted()      sets up NetworkIf, starts ADSR attack
 *   noteStopped()      releases bow, triggers ADSR release (tail-off)
 *   renderNextBlock()  produces audio; clears voice when RMS < threshold
 *
 * MPE per-note modulation:
 *   pressure       → p_bowForce
 *   pitchbend      → tuning (±48 semitones)
 *   timbre (CC74)  → p_nonlinearity
 */
class NetworkVoice  : public juce::MPESynthesiserVoice
{
public:
    enum class ExcitationMode { Bow, Pluck, Strike };

    NetworkVoice();

    //==========================================================================
    // juce::MPESynthesiserVoice overrides
    void noteStarted()                                                   override;
    void noteStopped (bool allowTailOff)                                 override;
    void notePressureChanged()                                           override;
    void notePitchbendChanged()                                          override;
    void noteTimbreChanged()                                             override;
    void noteKeyStateChanged()                                           override {}
    void setCurrentSampleRate (double newRate)                           override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples)               override;

    //==========================================================================
    void setExcitationMode (ExcitationMode mode) noexcept;
    void setEcoMode        (bool eco)            noexcept;

    /** Last computed block-level RMS — used by PhazonSynthesiser for voice stealing. */
    float getCurrentRMS() const noexcept { return rms_; }

private:
    NetworkIf  network_;
    juce::ADSR adsr_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoother_;

    ExcitationMode excitationMode_ = ExcitationMode::Bow;
    bool  ecoMode_  = false;
    bool  tailOff_  = false;
    float rms_      = 0.0f;

    std::vector<float> monoBuffer_;

    static constexpr float kRmsDeathThreshold = 1.0e-6f;
    static constexpr float kMpePitchBendRange = 48.0f;   ///< semitones per MPE spec

    void applyExcitationModeAtNoteStart();
};
