#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PhazonLookAndFeel.h"
#include <array>
#include <functional>
#include <cmath>

// =============================================================================
// StringVisualizer
//
// Animated 8-node displacement display driven by a 30 Hz timer.
// Caller provides an onFetchData callback that fills node positions + RMS.
// Energy level drives a glow: low → cyan, high → magenta.
// =============================================================================
class StringVisualizer : public juce::Component,
                         private juce::Timer
{
public:
    static constexpr int kNumNodes = 8;

    StringVisualizer()
    {
        nodes_.fill (0.0f);
        startTimerHz (30);
    }

    ~StringVisualizer() override { stopTimer(); }

    // -------------------------------------------------------------------------
    // Callback: void (float* nodesBuf8, int& countOut, float& rmsOut)
    // -------------------------------------------------------------------------
    std::function<void(float*, int&, float&)> onFetchData;

    // -------------------------------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        using namespace juce;

        const auto  b  = getLocalBounds().toFloat();
        const float w  = b.getWidth();
        const float h  = b.getHeight();
        const float cy = h * 0.5f;

        // BG
        g.setColour (Colour (0xff0a0a10));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (Colour (PhazonColors::PanelBorder));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

        // Energy 0..1 (already smoothed)
        const float e = jlimit (0.0f, 1.0f, energy_ * 60.0f);

        // Glow colour: cyan → magenta with energy
        const Colour cyanC (PhazonColors::Accent);
        const Colour magC  (PhazonColors::AccentWarm);
        const Colour glow = cyanC.interpolatedWith (magC, e);

        // Rest-line
        g.setColour (Colour (0xff181826));
        const float margin = w / (float)(kNumNodes + 1);
        g.drawHorizontalLine ((int)cy, margin, w - margin);

        // Build smooth curve through nodes via Catmull-Rom over [0, N-1]
        const int   segs     = 120;
        const float nodeSpan = (float)(kNumNodes - 1);
        const float dispPx   = h * 0.30f;   // max displacement in pixels

        Path curve;
        for (int s = 0; s <= segs; ++s)
        {
            const float t  = (float)s / (float)segs * nodeSpan;
            const float dy = catmullRom (nodes_.data(), kNumNodes, t);
            const float px = margin + t * (w - 2.0f * margin) / nodeSpan;
            const float py = cy - jlimit (-1.0f, 1.0f, dy) * dispPx;

            if (s == 0) curve.startNewSubPath (px, py);
            else        curve.lineTo (px, py);
        }

        // Glow layers (outer → inner)
        for (int layer = 5; layer >= 1; --layer)
        {
            const float thick = (float)layer * 2.8f;
            const float alpha = 0.05f * e / (float)layer;
            if (alpha < 0.004f) continue;
            g.setColour (glow.withAlpha (alpha));
            g.strokePath (curve, PathStrokeType (thick,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Core line
        g.setColour (glow.withAlpha (0.88f));
        g.strokePath (curve, PathStrokeType (1.6f,
            PathStrokeType::curved, PathStrokeType::rounded));

        // Node dots
        for (int i = 0; i < kNumNodes; ++i)
        {
            const float px  = margin + (float)i * (w - 2.0f * margin) / nodeSpan;
            const float val = jlimit (-1.0f, 1.0f, nodes_[i]);
            const float py  = cy - val * dispPx;
            const float vel = std::abs (val);
            const float dot = 2.2f + vel * 5.0f;

            // Dot halo
            g.setColour (glow.withAlpha (0.15f + e * 0.25f));
            g.fillEllipse (px - dot * 1.6f, py - dot * 1.6f, dot * 3.2f, dot * 3.2f);

            // Dot core
            g.setColour (glow);
            g.fillEllipse (px - dot * 0.45f, py - dot * 0.45f, dot * 0.9f, dot * 0.9f);
        }

        // "NODES" label (top-right, dim)
        g.setColour (Colour (PhazonColors::TextDim));
        g.setFont (juce::Font (9.0f));
        g.drawText (juce::String (kNumNodes) + " NODES",
                    (int)(w - 60), 4, 56, 12,
                    juce::Justification::centredRight);
    }

private:
    std::array<float, kNumNodes> nodes_ {};
    float energy_ = 0.0f;

    // -------------------------------------------------------------------------
    void timerCallback() override
    {
        if (onFetchData)
        {
            float  buf[kNumNodes] = {};
            int    count = 0;
            float  rms   = 0.0f;
            onFetchData (buf, count, rms);

            if (count > 0)
            {
                energy_ = energy_ * 0.82f + rms * 0.18f;
                const int n = std::min (count, kNumNodes);
                for (int i = 0; i < n; ++i)
                    nodes_[i] = nodes_[i] * 0.45f + buf[i] * 0.55f;
            }
        }

        repaint();
    }

    // -------------------------------------------------------------------------
    // Catmull-Rom interpolation on a flat float array of length n.
    // t ∈ [0, n-1]
    // -------------------------------------------------------------------------
    static float catmullRom (const float* arr, int n, float t) noexcept
    {
        const int  i1  = juce::jlimit (0, n - 1, (int) std::floor (t));
        const int  i0  = juce::jlimit (0, n - 1, i1 - 1);
        const int  i2  = juce::jlimit (0, n - 1, i1 + 1);
        const int  i3  = juce::jlimit (0, n - 1, i1 + 2);
        const float u  = t - (float) i1;
        const float u2 = u * u;
        const float u3 = u2 * u;
        return 0.5f * ((2.0f * arr[i1])
                     + (-arr[i0] + arr[i2]) * u
                     + (2.0f*arr[i0] - 5.0f*arr[i1] + 4.0f*arr[i2] - arr[i3]) * u2
                     + (-arr[i0] + 3.0f*arr[i1] - 3.0f*arr[i2] + arr[i3]) * u3);
    }
};
