#pragma once

#include <juce_dsp/juce_dsp.h>
#include "PhazonReverb.h"

// =============================================================================
//  CPU profiling bypass switches (compile-time, zero runtime cost).
//  Set any to 1 (e.g. -DPHAZON_BYPASS_DRIVE=1) to hard-skip that stage.
// =============================================================================
#ifndef PHAZON_BYPASS_DRIVE
 #define PHAZON_BYPASS_DRIVE  0
#endif
#ifndef PHAZON_BYPASS_FILTER
 #define PHAZON_BYPASS_FILTER 0
#endif
#ifndef PHAZON_BYPASS_SPACE
 #define PHAZON_BYPASS_SPACE  0
#endif
#ifndef PHAZON_BYPASS_GAIN
 #define PHAZON_BYPASS_GAIN   0
#endif

/**
 * PostProcessChain  — master post-processing inserted after synthesiser output.
 *
 * Signal flow:
 *   [Drive]   tanh waveshaper        (drive param 0..1, gain 1x..10x normalised)
 *   [Filter]  StateVariableTPT LP    (filterCutoff Hz, filterResonance 0..1 → Q)
 *   [Space]   Dattorro plate reverb  (spaceMix wet 0..1, spaceSize → decay)
 *   [Gain]    Output with SmoothedValue  (outputGainDb dB)
 *
 * All four stages are individually bypassable at compile-time via the
 * PHAZON_BYPASS_* macros above — set to 1 to profile CPU cost of each block.
 */
class PostProcessChain
{
public:
    // =========================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate_ = spec.sampleRate;

        // Each filter instance handles one channel — give it a mono spec.
        juce::dsp::ProcessSpec monoSpec = spec;
        monoSpec.numChannels = 1;
        for (auto& f : svFilter_)
        {
            f.prepare (monoSpec);
            f.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            f.setCutoffFrequency (20000.0f);
            f.setResonance (0.7071f);
        }

        reverb_.prepare (spec);

        // Pre-allocate gain scratch (avoids per-block heap alloc).
        gainScratch_.resize (spec.maximumBlockSize, 1.0f);
        wetBuffer_.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);

        gainSmoother_.reset (sampleRate_, 0.02);   // 20 ms ramp
        gainSmoother_.setCurrentAndTargetValue (1.0f);
    }

    void reset()
    {
        for (auto& f : svFilter_) f.reset();
        reverb_.reset();
        gainSmoother_.setCurrentAndTargetValue (gainSmoother_.getTargetValue());
    }

    // =========================================================================
    /**
     * Process a stereo (or mono) buffer in-place.
     *
     * @param buffer        Synthesiser output (modified in-place)
     * @param drive         0..1   — waveshaper gain 1x..10x
     * @param filterCutoff  Hz     — lowpass cutoff (20..20 000)
     * @param filterRes     0..1   — resonance, mapped to Q 0.707..10
     * @param spaceMix      0..1   — reverb wet amount
     * @param spaceSize     0..1   — reverb decay (0.50..0.9999)
     * @param outputGainDb  dB     — master output gain (-24..+12 typical)
     */
    void process (juce::AudioBuffer<float>& buffer,
                  float drive,
                  float filterCutoff,
                  float filterRes,
                  float spaceMix,
                  float spaceSize,
                  float outputGainDb,
                  const float* driveModBuffer = nullptr,
                  const float* cutoffModBuffer = nullptr,
                  int modulationSamples = 0)
    {
        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        // ---- Drive: tanh waveshaper -----------------------------------------
#if !PHAZON_BYPASS_DRIVE
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch);
                for (int n = 0; n < numSamples; ++n)
                {
                    const float driveValue = juce::jlimit (0.0f, 1.0f, drive + (driveModBuffer != nullptr && n < modulationSamples ? driveModBuffer[n] : 0.0f));
                    const float sampleGain = 1.0f + driveValue * 9.0f;
                    const float sampleNorm = 1.0f / std::tanh (sampleGain);
                    data[n] = std::tanh (data[n] * sampleGain) * sampleNorm;
                }
            }
        }
#endif

        // ---- Filter: StateVariableTPT lowpass --------------------------------
#if !PHAZON_BYPASS_FILTER
        {
            const float cutoff = juce::jlimit (20.0f, 20000.0f, filterCutoff);
            // filterRes 0..1  →  Q 0.7071..10
            const float Q = 0.7071f + filterRes * 9.2929f;

            for (int ch = 0; ch < numChannels && ch < 2; ++ch)
            {
                svFilter_[ch].setResonance (Q);

                auto* data = buffer.getWritePointer (ch);
                for (int n = 0; n < numSamples; ++n)
                {
                    const float cutoffOffset = cutoffModBuffer != nullptr && n < modulationSamples ? cutoffModBuffer[n] : 0.0f;
                    const float modulatedCutoff = juce::jlimit (20.0f, 20000.0f, cutoff * std::pow (2.0f, cutoffOffset * 4.0f));
                    svFilter_[ch].setCutoffFrequency (modulatedCutoff);
                    data[n] = svFilter_[ch].processSample (0, data[n]);
                }
            }
        }
#endif

        // ---- Space: Dattorro plate reverb + wet/dry blend -------------------
#if !PHAZON_BYPASS_SPACE
        if (spaceMix > 0.001f)
        {
            // spaceSize 0..1  →  decay 0.50..0.9999
            reverb_.setDecay (0.50f + spaceSize * 0.4999f);

            jassert (wetBuffer_.getNumChannels() >= numChannels);
            jassert (wetBuffer_.getNumSamples() >= numSamples);

            wetBuffer_.clear();
            for (int ch = 0; ch < numChannels; ++ch)
                wetBuffer_.copyFrom (ch, 0, buffer, ch, 0, numSamples);
            reverb_.process (wetBuffer_);

            // Crossfade: out = dry * (1 - mix) + wet * mix
            const float dry = 1.0f - spaceMix;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto*       dst = buffer.getWritePointer (ch);
                const auto* wet = wetBuffer_.getReadPointer (ch);
                for (int n = 0; n < numSamples; ++n)
                    dst[n] = dst[n] * dry + wet[n] * spaceMix;
            }
        }
#endif

        // ---- Gain: SmoothedValue output level --------------------------------
#if !PHAZON_BYPASS_GAIN
        {
            gainSmoother_.setTargetValue (
                juce::Decibels::decibelsToGain (outputGainDb));

            // Advance smoother once, cache ramp values — same ramp for all channels.
            const int scratch = juce::jmin (numSamples, (int) gainScratch_.size());
            for (int n = 0; n < scratch; ++n)
                gainScratch_[(size_t) n] = gainSmoother_.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch);
                for (int n = 0; n < scratch; ++n)
                    data[n] *= gainScratch_[(size_t) n];
            }
        }
#endif
    }

private:
    double sampleRate_ = 44100.0;

    // Two mono filter instances (L / R).
    juce::dsp::StateVariableTPTFilter<float> svFilter_[2];

    PhazonReverb reverb_;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoother_;

    // Pre-allocated gain ramp scratch (sized to maximumBlockSize in prepare()).
    std::vector<float> gainScratch_;
    juce::AudioBuffer<float> wetBuffer_;
};
