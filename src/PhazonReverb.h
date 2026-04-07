#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * Dattorro plate reverb.
 * J. Dattorro, "Effect Design Part 1: Reverberator and Other Filters",
 * JAES Vol.45 No.9, 1997.
 *
 * All delay times are scaled from the reference 29761 Hz sample rate.
 * Outputs fully-wet stereo; dry/wet blending is handled by the caller.
 */
class PhazonReverb
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sr    = spec.sampleRate;
        scale = (float) (sr / 29761.0);

        // 13ms headroom for predelay
        predelay.prepare ((int) (0.013 * sr) + 2);

        // Input diffusion chain (4 allpass)
        inAP[0].prepare ((int) (142 * scale + 0.5f));
        inAP[1].prepare ((int) (107 * scale + 0.5f));
        inAP[2].prepare ((int) (379 * scale + 0.5f));
        inAP[3].prepare ((int) (277 * scale + 0.5f));

        // Tank: modulated allpass (+8 samples headroom for ±6 mod)
        tankAP[0].prepare ((int) (672 * scale + 0.5f) + 8);
        tankAP[1].prepare ((int) (908 * scale + 0.5f) + 8);

        // Tank: main delay lines
        tankD[0].prepare ((int) (4453 * scale + 0.5f) + 1);
        tankD[1].prepare ((int) (3720 * scale + 0.5f) + 1);
        tankD[2].prepare ((int) (4217 * scale + 0.5f) + 1);
        tankD[3].prepare ((int) (3163 * scale + 0.5f) + 1);

        // Tank: static allpass (tapped for output)
        tankAP2[0].prepare ((int) (1800 * scale + 0.5f));
        tankAP2[1].prepare ((int) (2656 * scale + 0.5f));

        lfoInc = juce::MathConstants<float>::twoPi * 0.1f / (float) sr;

        reset();
    }

    void reset()
    {
        predelay.reset();
        for (auto& a : inAP)    a.reset();
        for (auto& a : tankAP)  a.reset();
        for (auto& a : tankAP2) a.reset();
        for (auto& d : tankD)   d.reset();
        bwState = dampState[0] = dampState[1] = 0.0f;
        nodeL = nodeR = 0.0f;
        lfoPhase = 0.0f;
    }

    /** Process buffer in-place (fully wet). */
    void process (juce::AudioBuffer<float>& buf)
    {
        const int numSamples = buf.getNumSamples();
        const int numCh      = juce::jmin (buf.getNumChannels(), 2);
        auto* L = buf.getWritePointer (0);
        auto* R = numCh > 1 ? buf.getWritePointer (1) : nullptr;

        const int pdLen = (int) (0.008 * sr); // 8 ms predelay

        for (int n = 0; n < numSamples; ++n)
        {
            float in = R ? (L[n] + R[n]) * 0.5f : L[n];

            // Predelay
            predelay.push (in);
            in = predelay.tap (pdLen);

            // Bandwidth filter (gentle input lowpass)
            bwState += 0.9995f * (in - bwState);
            in = bwState;

            // Input diffusion
            in = inAP[0].process (in, 0.750f);
            in = inAP[1].process (in, 0.750f);
            in = inAP[2].process (in, 0.625f);
            in = inAP[3].process (in, 0.625f);

            // LFO for tank modulation
            lfoPhase += lfoInc;
            if (lfoPhase >= juce::MathConstants<float>::twoPi)
                lfoPhase -= juce::MathConstants<float>::twoPi;
            const float lfo = std::sin (lfoPhase) * 6.0f;

            // ---- Left tank ----
            float tl = in + nodeR * decay;
            tl = tankAP[0].processModulated (tl, 0.70f, (int) (672 * scale), lfo);
            tankD[0].push (tl);
            tl = tankD[0].tap ((int) (4453 * scale));
            dampState[0] += (1.0f - damping) * (tl - dampState[0]);
            tl = dampState[0] * decay;
            tl = tankAP2[0].process (tl, 0.625f);
            tankD[1].push (tl);
            nodeL = tankD[1].tap ((int) (3720 * scale));

            // ---- Right tank ----
            float tr = in + nodeL * decay;
            tr = tankAP[1].processModulated (tr, 0.70f, (int) (908 * scale), -lfo);
            tankD[2].push (tr);
            tr = tankD[2].tap ((int) (4217 * scale));
            dampState[1] += (1.0f - damping) * (tr - dampState[1]);
            tr = dampState[1] * decay;
            tr = tankAP2[1].process (tr, 0.625f);
            tankD[3].push (tr);
            nodeR = tankD[3].tap ((int) (3163 * scale));

            // ---- Output taps (Dattorro Table 1) ----
            const float outL =   tankD[0].tap  ((int) (266  * scale))
                               + tankD[0].tap  ((int) (2974 * scale))
                               - tankAP2[0].tap((int) (1913 * scale))
                               + tankD[1].tap  ((int) (1996 * scale))
                               - tankD[2].tap  ((int) (1990 * scale))
                               - tankAP2[1].tap((int) (187  * scale))
                               - tankD[3].tap  ((int) (1066 * scale));

            const float outR =   tankD[2].tap  ((int) (353  * scale))
                               + tankD[2].tap  ((int) (3627 * scale))
                               - tankAP2[1].tap((int) (1228 * scale))
                               + tankD[3].tap  ((int) (2673 * scale))
                               - tankD[0].tap  ((int) (2111 * scale))
                               - tankAP2[0].tap((int) (335  * scale))
                               - tankD[1].tap  ((int) (121  * scale));

            L[n] = outL * 0.6f;
            if (R) R[n] = outR * 0.6f;
        }
    }

    void setDecay   (float d) noexcept { decay   = juce::jlimit (0.0f,    0.9999f, d); }
    void setDamping (float d) noexcept { damping = juce::jlimit (0.0001f, 0.9999f, d); }

private:
    //==========================================================================
    /** Simple circular delay line. tap(1) = last written, tap(k) = k steps back. */
    struct Delay
    {
        void prepare (int n) { buf.assign ((size_t) (n + 2), 0.0f); pos = 0; }
        void reset()         { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }

        void push (float v) noexcept
        {
            buf[(size_t) pos] = v;
            if (++pos >= (int) buf.size()) pos = 0;
        }

        float tap (int offset) const noexcept
        {
            const int n = (int) buf.size();
            return buf[(size_t) ((pos - 1 - (offset % n) + n * 2) % n)];
        }

        float tap (float offset) const noexcept
        {
            const int   i = (int) offset;
            const float f = offset - (float) i;
            return tap (i) * (1.0f - f) + tap (i + 1) * f;
        }

    private:
        std::vector<float> buf;
        int pos = 0;
    };

    //==========================================================================
    /** Schroeder allpass with optional fractional-delay modulation and internal tapping. */
    struct Allpass
    {
        void prepare (int n) { buf.assign ((size_t) n, 0.0f); writePos = 0; N = n; }
        void reset()         { std::fill (buf.begin(), buf.end(), 0.0f); writePos = 0; }

        /** Fixed-delay allpass (delay = N samples). */
        float process (float in, float g) noexcept
        {
            const float delayed  = buf[(size_t) writePos];
            const float feedback = in + g * delayed;
            buf[(size_t) writePos] = feedback;
            writePos = (writePos + 1) % N;
            return delayed - g * feedback;
        }

        /** Modulated allpass — delay oscillates around nominalDelay ± mod. */
        float processModulated (float in, float g, int nominalDelay, float mod) noexcept
        {
            float readF = (float) writePos - (float) nominalDelay - mod;
            while (readF <  0.0f) readF += (float) N;
            while (readF >= (float) N) readF -= (float) N;
            const int   r0 = (int) readF % N;
            const int   r1 = (r0 + 1) % N;
            const float fr = readF - (float) (int) readF;
            const float delayed  = buf[(size_t) r0] * (1.0f - fr) + buf[(size_t) r1] * fr;
            const float feedback = in + g * delayed;
            buf[(size_t) writePos] = feedback;
            writePos = (writePos + 1) % N;
            return delayed - g * feedback;
        }

        /** Tap the allpass's internal delay: offset steps before current write. */
        float tap (int offset) const noexcept
        {
            return buf[(size_t) (((writePos - 1 - (offset % N)) % N + N) % N)];
        }

    private:
        std::vector<float> buf;
        int writePos = 0;
        int N        = 0;
    };

    //==========================================================================
    double sr    = 44100.0;
    float  scale = 1.0f;

    Delay   predelay;
    Allpass inAP[4];
    Allpass tankAP[2];
    Allpass tankAP2[2];
    Delay   tankD[4];

    float bwState      = 0.0f;
    float dampState[2] = {};
    float nodeL        = 0.0f;
    float nodeR        = 0.0f;
    float lfoPhase     = 0.0f;
    float lfoInc       = 0.0f;
    float decay        = 0.72f;
    float damping      = 0.30f;
};
