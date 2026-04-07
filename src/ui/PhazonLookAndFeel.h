#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
// Palette
// =============================================================================
namespace PhazonColors
{
    inline constexpr juce::uint32 BG           = 0xff0d0d14;
    inline constexpr juce::uint32 PanelBG      = 0xff13131f;
    inline constexpr juce::uint32 PanelBorder  = 0xff1e1e30;
    inline constexpr juce::uint32 KnobBody     = 0xff1a1a2c;
    inline constexpr juce::uint32 KnobTrack    = 0xff252538;
    inline constexpr juce::uint32 Accent       = 0xff00e5ff;  // cyan
    inline constexpr juce::uint32 AccentWarm   = 0xffff44cc;  // magenta (high energy)
    inline constexpr juce::uint32 TextPrimary  = 0xffa0a0c0;
    inline constexpr juce::uint32 TextDim      = 0xff404058;
    inline constexpr juce::uint32 BtnActive    = 0xff162030;
    inline constexpr juce::uint32 HeaderBG     = 0xff0a0a11;
}

// =============================================================================
// PhazonLookAndFeel — dark, minimal, arc-knob style
// =============================================================================
class PhazonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PhazonLookAndFeel()
    {
        using namespace juce;

        // Sliders
        setColour (Slider::rotarySliderFillColourId,    Colour (PhazonColors::Accent));
        setColour (Slider::rotarySliderOutlineColourId, Colour (PhazonColors::KnobTrack));
        setColour (Slider::thumbColourId,               Colour (PhazonColors::Accent));
        setColour (Slider::backgroundColourId,          Colour (PhazonColors::KnobTrack));
        setColour (Slider::trackColourId,               Colour (PhazonColors::Accent));
        setColour (Slider::textBoxTextColourId,         Colour (PhazonColors::TextPrimary));
        setColour (Slider::textBoxBackgroundColourId,   Colour (0x00000000));
        setColour (Slider::textBoxOutlineColourId,      Colour (0x00000000));

        // Labels
        setColour (Label::textColourId,                 Colour (PhazonColors::TextPrimary));
        setColour (Label::backgroundColourId,           Colour (0x00000000));

        // Buttons
        setColour (TextButton::buttonColourId,          Colour (PhazonColors::PanelBG));
        setColour (TextButton::buttonOnColourId,        Colour (PhazonColors::BtnActive));
        setColour (TextButton::textColourOffId,         Colour (PhazonColors::TextPrimary));
        setColour (TextButton::textColourOnId,          Colour (PhazonColors::Accent));

        // ComboBox
        setColour (ComboBox::backgroundColourId,        Colour (PhazonColors::PanelBG));
        setColour (ComboBox::textColourId,              Colour (PhazonColors::TextPrimary));
        setColour (ComboBox::outlineColourId,           Colour (PhazonColors::PanelBorder));
        setColour (ComboBox::arrowColourId,             Colour (PhazonColors::Accent));
        setColour (ComboBox::focusedOutlineColourId,    Colour (PhazonColors::Accent));

        // ListBox
        setColour (ListBox::backgroundColourId,         Colour (PhazonColors::PanelBG));
        setColour (ListBox::textColourId,               Colour (PhazonColors::TextPrimary));
        setColour (ListBox::outlineColourId,            Colour (PhazonColors::PanelBorder));

        // TextEditor
        setColour (TextEditor::backgroundColourId,      Colour (PhazonColors::KnobBody));
        setColour (TextEditor::textColourId,            Colour (PhazonColors::TextPrimary));
        setColour (TextEditor::outlineColourId,         Colour (PhazonColors::PanelBorder));
        setColour (TextEditor::focusedOutlineColourId,  Colour (PhazonColors::Accent));
        setColour (TextEditor::highlightColourId,       Colour (PhazonColors::Accent).withAlpha (0.3f));

        // PopupMenu
        setColour (PopupMenu::backgroundColourId,              Colour (PhazonColors::PanelBG));
        setColour (PopupMenu::textColourId,                    Colour (PhazonColors::TextPrimary));
        setColour (PopupMenu::highlightedBackgroundColourId,   Colour (PhazonColors::BtnActive));
        setColour (PopupMenu::highlightedTextColourId,         Colour (PhazonColors::Accent));

        // ScrollBar
        setColour (ScrollBar::thumbColourId,            Colour (PhazonColors::KnobTrack));
    }

    // =========================================================================
    // Rotary knob — arc style, no pointer
    // =========================================================================
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& /*slider*/) override
    {
        using namespace juce;

        const float cx  = (float)x + (float)width  * 0.5f;
        const float cy  = (float)y + (float)height * 0.5f;
        const float rad = std::min ((float)width, (float)height) * 0.5f - 3.0f;

        // Body circle
        g.setColour (Colour (PhazonColors::KnobBody));
        g.fillEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);

        // Rim
        g.setColour (Colour (PhazonColors::PanelBorder));
        g.drawEllipse (cx - rad + 0.5f, cy - rad + 0.5f,
                       (rad - 0.5f) * 2.0f, (rad - 0.5f) * 2.0f, 1.0f);

        const float arcRad = rad - 4.0f;

        // Track arc (full range, dim)
        {
            Path track;
            track.addCentredArc (cx, cy, arcRad, arcRad,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (Colour (PhazonColors::KnobTrack));
            g.strokePath (track, PathStrokeType (2.4f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Value arc
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        {
            Path value;
            value.addCentredArc (cx, cy, arcRad, arcRad,
                                 0.0f, rotaryStartAngle, angle, true);
            g.setColour (Colour (PhazonColors::Accent));
            g.strokePath (value, PathStrokeType (2.4f,
                PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Tip dot
        {
            const float angle90 = angle - MathConstants<float>::halfPi;
            const float tx = cx + (arcRad - 1.0f) * std::cos (angle90);
            const float ty = cy + (arcRad - 1.0f) * std::sin (angle90);
            g.setColour (Colour (PhazonColors::Accent));
            g.fillEllipse (tx - 2.5f, ty - 2.5f, 5.0f, 5.0f);
        }

        // Centre dot
        g.setColour (Colour (PhazonColors::KnobTrack));
        g.fillEllipse (cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
    }

    // =========================================================================
    // Linear slider — thin track
    // =========================================================================
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float /*minPos*/, float /*maxPos*/,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        using namespace juce;

        if (style == Slider::LinearHorizontal || style == Slider::LinearBar)
        {
            const float cy = (float)y + (float)height * 0.5f;
            const float r  = 3.0f;

            // Track
            g.setColour (Colour (PhazonColors::KnobTrack));
            g.fillRoundedRectangle ((float)x, cy - 2.0f, (float)width, 4.0f, 2.0f);

            // Fill to position
            g.setColour (Colour (PhazonColors::Accent));
            g.fillRoundedRectangle ((float)x, cy - 2.0f, sliderPos - (float)x, 4.0f, 2.0f);

            // Thumb
            g.setColour (Colour (PhazonColors::Accent));
            g.fillEllipse (sliderPos - r, cy - r, r * 2.0f, r * 2.0f);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                                              sliderPos, 0.0f, 0.0f, style, slider);
        }
    }

    // =========================================================================
    // Button background
    // =========================================================================
    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& /*bg*/,
                               bool isHighlighted,
                               bool isDown) override
    {
        using namespace juce;

        const auto  bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool  on     = button.getToggleState();

        Colour fill = on ? Colour (PhazonColors::Accent).withAlpha (0.18f)
                         : Colour (PhazonColors::PanelBG);
        if (isHighlighted) fill = fill.brighter (0.06f);
        if (isDown)        fill = fill.brighter (0.12f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (on ? Colour (PhazonColors::Accent).withAlpha (0.85f)
                        : Colour (PhazonColors::PanelBorder));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool /*isHighlighted*/,
                         bool /*isDown*/) override
    {
        using namespace juce;
        const bool on = button.getToggleState();
        g.setColour (on ? Colour (PhazonColors::Accent) : Colour (PhazonColors::TextPrimary));
        g.setFont (Font (10.0f, Font::bold));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds(), Justification::centred, 1);
    }

    // =========================================================================
    // ComboBox
    // =========================================================================
    void drawComboBox (juce::Graphics& g, int width, int height,
                       bool /*isDown*/, int, int, int, int,
                       juce::ComboBox& /*box*/) override
    {
        using namespace juce;
        const auto r = Rectangle<float> (0.0f, 0.0f, (float)width, (float)height);
        g.setColour (Colour (PhazonColors::PanelBG));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (Colour (PhazonColors::PanelBorder));
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

        // Arrow
        const float ax = (float)width - 14.0f;
        const float ay = (float)height * 0.5f;
        Path arrow;
        arrow.addTriangle (ax, ay - 3.0f, ax + 7.0f, ay - 3.0f, ax + 3.5f, ay + 3.0f);
        g.setColour (Colour (PhazonColors::Accent));
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox& /*box*/) override
    {
        return juce::Font (11.0f);
    }

    // =========================================================================
    // Label
    // =========================================================================
    juce::Font getLabelFont (juce::Label& label) override
    {
        return label.getFont();
    }

    // =========================================================================
    // ScrollBar
    // =========================================================================
    void drawScrollbar (juce::Graphics& g,
                        juce::ScrollBar& scrollbar,
                        int x, int y, int width, int height,
                        bool /*isScrollbarVertical*/,
                        int thumbStartPosition, int thumbSize,
                        bool /*isMouseOver*/, bool /*isMouseDown*/) override
    {
        using namespace juce;
        g.setColour (Colour (PhazonColors::PanelBG));
        g.fillRect (x, y, width, height);

        g.setColour (Colour (PhazonColors::KnobTrack));
        g.fillRoundedRectangle ((float)(x + 2), (float)(y + thumbStartPosition + 2),
                                (float)(width - 4), (float)(thumbSize - 4), 3.0f);
    }
};
