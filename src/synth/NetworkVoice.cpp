#include "NetworkVoice.h"

#include <cmath>

NetworkVoice::NetworkVoice()
{
    for (auto& node : visualNodes_)
        node.store (0.0f, std::memory_order_relaxed);

    juce::ADSR::Parameters p;
    p.attack  = 0.005f;   // 5 ms — smooths onset transient
    p.decay   = 0.0f;
    p.sustain = 1.0f;
    p.release = 0.300f;   // 300 ms release tail
    adsr_.setParameters (p);
    adsr_.setSampleRate (44100.0);

    gainSmoother_.reset (44100, 0.020);   // 20 ms gain ramp
    gainSmoother_.setCurrentAndTargetValue (0.0f);

    random_.setSeedRandomly();
    lfoRandomValue_ = random_.nextFloat() * 2.0f - 1.0f;
    lfoRandomNextValue_ = random_.nextFloat() * 2.0f - 1.0f;
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

void NetworkVoice::prepare (int maximumBlockSize)
{
    monoBuffer_.assign ((size_t) maximumBlockSize, 0.0f);
    driveModBuffer_.assign ((size_t) maximumBlockSize, 0.0f);
    cutoffModBuffer_.assign ((size_t) maximumBlockSize, 0.0f);
}

void NetworkVoice::setBaseParams (const PhysicsParams& params) noexcept
{
    baseParams_ = params;

    if (!ecoMode_)
    {
        const auto profile = params.massProfile == 0
                               ? NetworkIf::Profile::OneDimensional
                               : NetworkIf::Profile::TwoDimensional;
        network_.changeProfile (profile);
    }

    // Push base values into the network; MPE overrides are re-applied per event
    network_.setModulationBaseValues (params.springDamping,
                                      params.massDamping,
                                      params.bowForce,
                                      params.nonlinearity,
                                      params.excitationRatio,
                                      0.70f,
                                      params.chaos);
    network_.p_springDamping   = params.springDamping;
    network_.p_massDamping     = params.massDamping;
    network_.p_bowForce        = params.bowForce;
    network_.p_nonlinearity    = params.nonlinearity;
    network_.p_excitationRatio = params.excitationRatio;
    network_.p_pickupBase      = 0.70f;
    network_.p_bowWidth        = params.bowWidth;
    network_.p_octave          = params.octave;
    network_.p_detune          = params.detune;
    network_.p_chaos           = params.chaos;

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

void NetworkVoice::setModMatrixConfig (const ModMatrix::Config& config) noexcept
{
    modMatrix_ = config;
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
    noteVelocity_ = velocity;
    notePressure_ = note.pressure.asUnsignedFloat();
    noteTimbre_ = note.timbre.asUnsignedFloat();
    const float spread = juce::jlimit (-0.15f, 0.15f, -0.15f + 0.30f * ((float) voiceIndex_ / 7.0f));
    lfoPhase_ = spread < 0.0f ? spread + 1.0f : spread;
    lfoRandomValue_ = random_.nextFloat() * 2.0f - 1.0f;
    lfoRandomNextValue_ = random_.nextFloat() * 2.0f - 1.0f;

    adsr_.noteOn();
    tailOff_ = false;
    rms_.store (0.0f, std::memory_order_relaxed);

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
    notePressure_ = getCurrentlyPlayingNote().pressure.asUnsignedFloat();
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
    noteTimbre_ = getCurrentlyPlayingNote().timbre.asUnsignedFloat();
}

//==============================================================================
// Audio render
//==============================================================================

void NetworkVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                    int startSample, int numSamples)
{
    jassert ((int) monoBuffer_.size() >= numSamples);
    jassert ((int) driveModBuffer_.size() >= numSamples);
    jassert ((int) cutoffModBuffer_.size() >= numSamples);

    for (int i = 0; i < numSamples; ++i)
        monoBuffer_[(size_t) i] = processSample (i);

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

    rms_.store (std::sqrt (rmsAcc / (float)numSamples), std::memory_order_relaxed);
    updateVisualSnapshot();

    // Voice death: ADSR finished + energy below threshold
    if (tailOff_ && !adsr_.isActive() && getCurrentRMS() < kRmsDeathThreshold)
        clearCurrentNote();
}

bool NetworkVoice::contributesToPostModulation() const noexcept
{
    return getCurrentlyPlayingNote().isValid() || getCurrentRMS() > kRmsDeathThreshold;
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

float NetworkVoice::processSample (int sampleIndex)
{
    ModMatrix::DeltaFrame modDelta {};
    const float lfoValue = renderMatrixLfo();

    for (const auto& slot : modMatrix_.slots)
    {
        if (slot.source == ModMatrix::Source::Off || slot.destination == ModMatrix::Destination::Off)
            continue;

        modDelta[(size_t) slot.destination] += getSourceValue (slot.source, lfoValue) * slot.amount;
    }
    driveModBuffer_[(size_t) sampleIndex] = modDelta[(size_t) ModMatrix::Destination::Drive];
    cutoffModBuffer_[(size_t) sampleIndex] = modDelta[(size_t) ModMatrix::Destination::Cutoff];

    return network_.processSample (modDelta);
}

float NetworkVoice::renderMatrixLfo()
{
    const float phase = lfoPhase_;
    const float wrappedPhase = phase - std::floor (phase);

    float value = 0.0f;
    switch (modMatrix_.lfoShape)
    {
        case 0: value = std::sin (juce::MathConstants<float>::twoPi * wrappedPhase); break;
        case 1: value = wrappedPhase < 0.5f ? 1.0f : -1.0f; break;
        case 2: value = lfoRandomValue_ + (lfoRandomNextValue_ - lfoRandomValue_) * wrappedPhase; break;
        case 3: value = 1.0f - 4.0f * std::abs (wrappedPhase - 0.5f); break;
        default: break;
    }

    const float increment = modMatrix_.lfoRate / (float) getSampleRate();
    const float nextPhase = wrappedPhase + increment;
    if (nextPhase >= 1.0f)
    {
        lfoPhase_ = nextPhase - std::floor (nextPhase);
        lfoRandomValue_ = lfoRandomNextValue_;
        lfoRandomNextValue_ = random_.nextFloat() * 2.0f - 1.0f;
    }
    else
    {
        lfoPhase_ = nextPhase;
    }

    return value * modMatrix_.lfoDepth;
}

float NetworkVoice::getSourceValue (ModMatrix::Source source, float lfoValue) const noexcept
{
    switch (source)
    {
        case ModMatrix::Source::Lfo:      return lfoValue;
        case ModMatrix::Source::Pressure: return notePressure_;
        case ModMatrix::Source::Timbre:   return noteTimbre_;
        case ModMatrix::Source::Velocity: return noteVelocity_;
        case ModMatrix::Source::Off:      break;
    }

    return 0.0f;
}

void NetworkVoice::getVisualSnapshot (float* nodes, int& count, float& rms) const noexcept
{
    count = (int) visualNodes_.size();
    rms = getCurrentRMS();

    for (int i = 0; i < count; ++i)
        nodes[i] = visualNodes_[i].load (std::memory_order_relaxed);
}

void NetworkVoice::updateVisualSnapshot() noexcept
{
    if (network_.x.empty())
    {
        for (auto& node : visualNodes_)
            node.store (0.0f, std::memory_order_relaxed);
        return;
    }

    const int sourceCount = (int) network_.x.size();
    const int targetCount = (int) visualNodes_.size();

    if (sourceCount == 1)
    {
        const float value = network_.x.front();
        for (auto& node : visualNodes_)
            node.store (value, std::memory_order_relaxed);
        return;
    }

    for (int i = 0; i < targetCount; ++i)
    {
        const float t = (float) i * (float) (sourceCount - 1) / (float) (targetCount - 1);
        const int i0 = juce::jlimit (0, sourceCount - 1, (int) std::floor (t));
        const int i1 = juce::jlimit (0, sourceCount - 1, i0 + 1);
        const float frac = t - (float) i0;
        const float value = juce::jmap (frac, network_.x[(size_t) i0], network_.x[(size_t) i1]);
        visualNodes_[(size_t) i].store (juce::jlimit (-1.0f, 1.0f, value * 6.0f),
                                        std::memory_order_relaxed);
    }
}
