#include "PhazonSynthesiser.h"

#include <limits>

PhazonSynthesiser::PhazonSynthesiser()
{
    // Allocate max polyphony upfront; voices are reused across notes
    for (int i = 0; i < kDefaultPolyphony; ++i)
    {
        auto* voice = new NetworkVoice();
        voice->setVoiceIndex (i);
        addVoice (voice);
    }

    setVoiceStealingEnabled (true);

    // Standard MPE layout: lower zone, 15 member channels (ch 2–16), master ch 1
    juce::MPEZoneLayout layout;
    layout.setLowerZone (15);
    instrument.setZoneLayout (layout);
}

//==============================================================================
void PhazonSynthesiser::setEcoMode (bool eco)
{
    ecoMode_        = eco;
    activeVoiceCap_ = eco ? kEcoPolyphony : kDefaultPolyphony;

    for (int i = 0; i < getNumVoices(); ++i)
        if (auto* v = dynamic_cast<NetworkVoice*> (getVoice (i)))
            v->setEcoMode (eco);
}

void PhazonSynthesiser::setExcitationMode (NetworkVoice::ExcitationMode mode)
{
    excitationMode_ = mode;
    for (int i = 0; i < getNumVoices(); ++i)
        if (auto* v = dynamic_cast<NetworkVoice*> (getVoice (i)))
            v->setExcitationMode (mode);
}

void PhazonSynthesiser::setBaseParams (const PhysicsParams& params)
{
    for (int i = 0; i < getNumVoices(); ++i)
        if (auto* v = dynamic_cast<NetworkVoice*> (getVoice (i)))
            v->setBaseParams (params);
}

void PhazonSynthesiser::setModMatrixConfig (const ModMatrix::Config& config)
{
    modMatrix_ = config;

    for (int i = 0; i < getNumVoices(); ++i)
        if (auto* v = dynamic_cast<NetworkVoice*> (getVoice (i)))
            v->setModMatrixConfig (config);
}

void PhazonSynthesiser::prepare (int maximumBlockSize)
{
    driveModBuffer_.assign ((size_t) maximumBlockSize, 0.0f);
    cutoffModBuffer_.assign ((size_t) maximumBlockSize, 0.0f);

    for (int i = 0; i < getNumVoices(); ++i)
        if (auto* v = dynamic_cast<NetworkVoice*> (getVoice (i)))
            v->prepare (maximumBlockSize);
}

void PhazonSynthesiser::renderNextBlock (juce::AudioBuffer<float>& outputAudio,
                                         const juce::MidiBuffer& inputMidi,
                                         int startSample,
                                         int numSamples)
{
    juce::MPESynthesiser::renderNextBlock (outputAudio, inputMidi, startSample, numSamples);
    aggregatePostModulation (numSamples);
}

void PhazonSynthesiser::getVisualizerData (float* nodes, int& count, float& rms) const noexcept
{
    count = 8;
    rms = 0.0f;

    for (int i = 0; i < count; ++i)
        nodes[i] = 0.0f;

    const NetworkVoice* bestVoice = nullptr;
    float bestRms = 0.0f;

    for (int i = 0; i < activeVoiceCap_; ++i)
    {
        if (auto* voice = dynamic_cast<NetworkVoice*> (getVoice (i)))
        {
            const float voiceRms = voice->getCurrentRMS();
            if (voiceRms > bestRms)
            {
                bestRms = voiceRms;
                bestVoice = voice;
            }
        }
    }

    if (bestVoice != nullptr)
        bestVoice->getVisualSnapshot (nodes, count, rms);
}

//==============================================================================
juce::MPESynthesiserVoice* PhazonSynthesiser::findVoiceToSteal (
    juce::MPENote /*noteToStealVoiceFor*/) const
{
    // 1. Prefer an idle voice within the active polyphony cap
    for (int i = 0; i < activeVoiceCap_; ++i)
    {
        auto* v = getVoice (i);
        if (!v->getCurrentlyPlayingNote().isValid())
            return v;
    }

    // 2. Steal the lowest-energy playing voice within the cap
    juce::MPESynthesiserVoice* candidate = nullptr;
    float lowestRms = std::numeric_limits<float>::max();

    for (int i = 0; i < activeVoiceCap_; ++i)
    {
        auto* v = getVoice (i);
        if (auto* nv = dynamic_cast<NetworkVoice*> (v))
        {
            if (nv->getCurrentRMS() < lowestRms)
            {
                lowestRms = nv->getCurrentRMS();
                candidate = nv;
            }
        }
        else if (candidate == nullptr)
        {
            candidate = v;   // fallback for non-NetworkVoice (shouldn't happen)
        }
    }

    return candidate;
}

void PhazonSynthesiser::aggregatePostModulation (int numSamples)
{
    jassert ((int) driveModBuffer_.size() >= numSamples);
    jassert ((int) cutoffModBuffer_.size() >= numSamples);
    std::fill (driveModBuffer_.begin(), driveModBuffer_.begin() + numSamples, 0.0f);
    std::fill (cutoffModBuffer_.begin(), cutoffModBuffer_.begin() + numSamples, 0.0f);

    int contributingVoices = 0;

    for (int i = 0; i < activeVoiceCap_; ++i)
    {
        auto* rawVoice = getVoice (i);
        auto* voice = dynamic_cast<NetworkVoice*> (rawVoice);
        if (voice == nullptr || !voice->contributesToPostModulation())
            continue;

        const auto& drive = voice->getDriveModulationBuffer();
        const auto& cutoff = voice->getCutoffModulationBuffer();
        if ((int) drive.size() < numSamples || (int) cutoff.size() < numSamples)
            continue;

        ++contributingVoices;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            driveModBuffer_[(size_t) sample] += drive[(size_t) sample];
            cutoffModBuffer_[(size_t) sample] += cutoff[(size_t) sample];
        }
    }

    if (contributingVoices <= 1)
        return;

    const float scale = 1.0f / (float) contributingVoices;
    for (int sample = 0; sample < numSamples; ++sample)
    {
        driveModBuffer_[(size_t) sample] *= scale;
        cutoffModBuffer_[(size_t) sample] *= scale;
    }
}
