#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/NetworkIf.h"
#include "ModMatrix.h"
#include <array>
#include <atomic>

/**
 * Snapshot of base (non-MPE) parameters pushed from the processor each block.
 * All values are in raw (un-normalised) parameter units.
 */
struct PhysicsParams
{
    float springDamping   = 0.20f;
    float massDamping     = 0.02f;
    float bowForce        = 0.50f;
    float nonlinearity    = 0.25f;
    float excitationRatio = 0.30f;
    float bowWidth        = 0.05f;
    float drive           = 0.30f;
    int   dimensions      = 8;
    int   massProfile     = 0;
    float attack          = 0.005f;
    float release         = 0.300f;
    float octave          = 0.0f;
    float detune          = 0.0f;
    float chaos           = 0.0f;
};

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
    void setExcitationMode (ExcitationMode mode)         noexcept;
    void setEcoMode        (bool eco)                    noexcept;

    /** Called once per block by PhazonSynthesiser to push APVTS base values.
     *  MPE per-note deltas (pressure, pitchbend, timbre) are still applied
     *  on top in the notePressureChanged / noteTimbreChanged callbacks. */
    void setBaseParams     (const PhysicsParams& params) noexcept;
    void setModMatrixConfig (const ModMatrix::Config& config) noexcept;
    void setVoiceIndex      (int index) noexcept { voiceIndex_ = index; }
    void prepare (int maximumBlockSize);

    /** Last computed block-level RMS — used by PhazonSynthesiser for voice stealing. */
    float getCurrentRMS() const noexcept { return rms_.load (std::memory_order_relaxed); }

    /** Returns the latest visual snapshot for the editor visualiser. */
    void getVisualSnapshot (float* nodes, int& count, float& rms) const noexcept;
    const std::vector<float>& getDriveModulationBuffer() const noexcept  { return driveModBuffer_; }
    const std::vector<float>& getCutoffModulationBuffer() const noexcept { return cutoffModBuffer_; }
    bool contributesToPostModulation() const noexcept;

private:
    NetworkIf  network_;
    juce::ADSR adsr_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoother_;

    PhysicsParams  baseParams_;
    ModMatrix::Config modMatrix_;
    ExcitationMode excitationMode_ = ExcitationMode::Bow;
    bool  ecoMode_  = false;
    bool  tailOff_  = false;
    int   voiceIndex_ = 0;
    float noteVelocity_ = 0.0f;
    float notePressure_ = 0.0f;
    float noteTimbre_ = 0.0f;
    float lfoPhase_ = 0.0f;
    float lfoRandomValue_ = 0.0f;
    float lfoRandomNextValue_ = 0.0f;
    juce::Random random_;
    std::atomic<float> rms_ { 0.0f };
    std::array<std::atomic<float>, 8> visualNodes_ {};

    std::vector<float> monoBuffer_;
    std::vector<float> driveModBuffer_;
    std::vector<float> cutoffModBuffer_;

    static constexpr float kRmsDeathThreshold = 1.0e-6f;
    static constexpr float kMpePitchBendRange = 48.0f;   ///< semitones per MPE spec

    void applyExcitationModeAtNoteStart();
    float processSample (int sampleIndex);
    float renderMatrixLfo();
    float getSourceValue (ModMatrix::Source source, float lfoValue) const noexcept;
    void updateVisualSnapshot() noexcept;
};
