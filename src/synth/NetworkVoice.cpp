#include "NetworkVoice.h"

#include <cmath>

NetworkVoice::NetworkVoice()
{
    juce::ADSR::Parameters p;
    p.attack  = 0.005f;   // 5 ms — smooths onset transient
    p.decay   = 0.0f;
    p.sustain = 1.0f;
    p.release = 0.300f;   // 300 ms release tail
    adsr_.setParameters (p);
    adsr_.setSampleRate (44100.0);

    gainSmoother_.reset (44100, 0.020);   // 20 ms gain ramp
    gainSmoother_.setCurrentAndTargetValue (0.0f);
}

//==============================================================================
void NetworkVoice::setCurrentSampleRate (double newRate)
{
    juce::MPESynthesiserVoice::setCurrentSampleRate (newRate);
    adsr_.setSampleRate (newRate);
    gainSmoother_.reset ((int)newRate, 0.020);
    network_.setSampleRate ((float)newRate);
}

void NetworkVoice::setExcitationMode (ExcitationMode mode) noexcept
{
    excitationMode_ = mode;
}

void NetworkVoice::setBaseParams (const PhysicsParams& params) noexcept
{
    baseParams_ = params;

    // Push base values into the network; MPE overrides are re-applied per event
    network_.p_springDamping   = params.springDamping;
    network_.p_massDamping     = params.massDamping;
    network_.p_bowForce        = params.bowForce;
    network_.p_nonlinearity    = params.nonlinearity;
    network_.p_excitationRatio = params.excitationRatio;
    network_.p_bowWidth        = params.bowWidth;
    network_.p_octave          = params.octave;
    network_.p_detune          = params.detune;

    if (network_.n_total != params.dimensions)
        network_.changeDimensions (params.dimensions);

    juce::ADSR::Parameters adsrP;
    adsrP.attack  = params.attack;
    adsrP.decay   = 0.0f;
    adsrP.sustain = 1.0f;
    adsrP.release = params.release;
    adsr_.setParameters (adsrP);

    network_.refreshCoefficients();
}

void NetworkVoice::setEcoMode (bool eco) noexcept
{
    ecoMode_ = eco;
    network_.changeProfile (eco ? NetworkIf::Profile::Eco
                                : NetworkIf::Profile::OneDimensional);
}

//==============================================================================
// Note lifecycle
//==============================================================================

void NetworkVoice::noteStarted()
{
    const auto  note     = getCurrentlyPlayingNote();
    const float velocity = note.noteOnVelocity.asUnsignedFloat();
    const float midiNote = (float)note.initialNote;
    const float bend     = note.pitchbend.asSignedFloat() * kMpePitchBendRange;

    network_.setupNote (midiNote, velocity, bend, /*isMono=*/false);

    adsr_.noteOn();
    tailOff_ = false;
    rms_     = 0.0f;

    gainSmoother_.setCurrentAndTargetValue (0.0f);
    gainSmoother_.setTargetValue (1.0f);

    applyExcitationModeAtNoteStart();
}

void NetworkVoice::noteStopped (bool allowTailOff)
{
    network_.stopNote();   // zeroes bow velocity so excitation stops
    adsr_.noteOff();
    tailOff_ = true;

    if (!allowTailOff)
        clearCurrentNote();
}

//==============================================================================
// MPE per-note modulation
//==============================================================================

void NetworkVoice::notePressureChanged()
{
    // Pressure (slide / aftertouch) → bow force
    const float pressure = getCurrentlyPlayingNote().pressure.asUnsignedFloat();
    network_.p_bowForce = pressure;
    network_.refreshCoefficients();
}

void NetworkVoice::notePitchbendChanged()
{
    const auto  note     = getCurrentlyPlayingNote();
    const float midiNote = (float)note.initialNote;
    const float bend     = note.pitchbend.asSignedFloat() * kMpePitchBendRange;
    network_.updatePitch (midiNote, bend);   // no state reset
}

void NetworkVoice::noteTimbreChanged()
{
    // CC74 (timbre / brightness) → nonlinearity of friction curve
    const float timbre = getCurrentlyPlayingNote().timbre.asUnsignedFloat();
    network_.p_nonlinearity = timbre;
    network_.refreshCoefficients();
}

//==============================================================================
// Audio render
//==============================================================================

void NetworkVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                    int startSample, int numSamples)
{
    if (monoBuffer_.size() < (size_t)numSamples)
        monoBuffer_.resize ((size_t)numSamples);

    // Run physics
    network_.renderNextBlock (monoBuffer_.data(), numSamples);

    // Apply envelope + gain smoother; accumulate RMS
    float rmsAcc = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float env  = adsr_.getNextSample();
        const float gain = gainSmoother_.getNextValue();
        const float s    = monoBuffer_[i] * env * gain;
        rmsAcc += s * s;

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample (ch, startSample + i, s);
    }

    rms_ = std::sqrt (rmsAcc / (float)numSamples);

    // Voice death: ADSR finished + energy below threshold
    if (tailOff_ && !adsr_.isActive() && rms_ < kRmsDeathThreshold)
        clearCurrentNote();
}

//==============================================================================
// Excitation mode dispatch
//==============================================================================

void NetworkVoice::applyExcitationModeAtNoteStart()
{
    switch (excitationMode_)
    {
        case ExcitationMode::Bow:
            // bowVelocity already set proportionally to MIDI velocity by setupNote
            break;

        case ExcitationMode::Pluck:
        {
            // Displace the string at the excitation position, then release bow
            const float pos = network_.p_excitationRatio * float (network_.n_total - 1);
            const float amp = network_.bowVelocity * 0.01f;
            NetworkIf::interpolation3Mult (network_.x.data(), network_.n_total, pos, amp);
            network_.bowVelocity = 0.0f;
            break;
        }

        case ExcitationMode::Strike:
        {
            // Wider impulse centred at midpoint — models a mallet/hammer strike
            const float mid = float (network_.n_total - 1) * 0.5f;
            const float amp = network_.bowVelocity * 0.015f;
            NetworkIf::interpolation3Mult (network_.x.data(), network_.n_total, mid - 1.0f, amp * 0.5f);
            NetworkIf::interpolation3Mult (network_.x.data(), network_.n_total, mid,         amp);
            NetworkIf::interpolation3Mult (network_.x.data(), network_.n_total, mid + 1.0f, amp * 0.5f);
            network_.bowVelocity = 0.0f;
            break;
        }
    }
}
