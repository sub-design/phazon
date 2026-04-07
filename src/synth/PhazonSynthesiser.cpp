#include "PhazonSynthesiser.h"

#include <limits>

PhazonSynthesiser::PhazonSynthesiser()
{
    // Allocate max polyphony upfront; voices are reused across notes
    for (int i = 0; i < kDefaultPolyphony; ++i)
        addVoice (new NetworkVoice());

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
