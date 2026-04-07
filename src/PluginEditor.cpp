#include "PluginEditor.h"

#include "ParameterIDs.h"
#include "synth/ModMatrix.h"
#include "ui/PhazonLookAndFeel.h"
#include "ui/PresetBrowser.h"
#include "ui/StringVisualizer.h"

namespace
{
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

class Panel : public juce::Component
{
public:
    explicit Panel (juce::String title) : title_ (std::move (title)) {}

    juce::Rectangle<int> getContentBounds() const
    {
        return getLocalBounds().reduced (14, 14).withTrimmedTop (24);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour (juce::Colour (PhazonColors::PanelBG));
        g.fillRoundedRectangle (bounds, 12.0f);

        g.setColour (juce::Colour (PhazonColors::PanelBorder));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

        auto header = bounds.removeFromTop (28.0f);
        g.setColour (juce::Colour (PhazonColors::HeaderBG).withAlpha (0.65f));
        g.fillRoundedRectangle (header, 12.0f);

        g.setFont (juce::Font (12.5f, juce::Font::bold));
        g.setColour (juce::Colour (PhazonColors::TextPrimary));
        g.drawText (title_.toUpperCase(), header.toNearestInt().reduced (12, 0),
                    juce::Justification::centredLeft);
    }

private:
    juce::String title_;
};

class KnobControl : public juce::Component
{
public:
    explicit KnobControl (juce::String title)
    {
        titleLabel_.setText (std::move (title), juce::dontSendNotification);
        titleLabel_.setJustificationType (juce::Justification::centred);
        titleLabel_.setFont (juce::Font (11.0f, juce::Font::bold));
        titleLabel_.setColour (juce::Label::textColourId, juce::Colour (PhazonColors::TextPrimary));
        addAndMakeVisible (titleLabel_);

        slider_.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 18);
        slider_.setScrollWheelEnabled (false);
        addAndMakeVisible (slider_);
    }

    juce::Slider& slider() noexcept { return slider_; }

    void resized() override
    {
        auto area = getLocalBounds();
        titleLabel_.setBounds (area.removeFromTop (18));
        slider_.setBounds (area);
    }

private:
    juce::Label titleLabel_;
    juce::Slider slider_;
};

class SliderControl : public juce::Component
{
public:
    explicit SliderControl (juce::String title)
    {
        titleLabel_.setText (std::move (title), juce::dontSendNotification);
        titleLabel_.setFont (juce::Font (10.0f));
        titleLabel_.setColour (juce::Label::textColourId, juce::Colour (PhazonColors::TextPrimary));
        addAndMakeVisible (titleLabel_);

        slider_.setSliderStyle (juce::Slider::LinearHorizontal);
        slider_.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 18);
        slider_.setScrollWheelEnabled (false);
        addAndMakeVisible (slider_);
    }

    juce::Slider& slider() noexcept { return slider_; }

    void resized() override
    {
        auto area = getLocalBounds();
        titleLabel_.setBounds (area.removeFromTop (16));
        slider_.setBounds (area.removeFromTop (22));
    }

private:
    juce::Label titleLabel_;
    juce::Slider slider_;
};

class ChoiceControl : public juce::Component
{
public:
    explicit ChoiceControl (juce::String title)
    {
        titleLabel_.setText (std::move (title), juce::dontSendNotification);
        titleLabel_.setFont (juce::Font (10.0f));
        titleLabel_.setColour (juce::Label::textColourId, juce::Colour (PhazonColors::TextPrimary));
        addAndMakeVisible (titleLabel_);
        addAndMakeVisible (combo_);
    }

    juce::ComboBox& combo() noexcept { return combo_; }

    void resized() override
    {
        auto area = getLocalBounds();
        titleLabel_.setBounds (area.removeFromTop (16));
        combo_.setBounds (area.removeFromTop (24));
    }

private:
    juce::Label titleLabel_;
    juce::ComboBox combo_;
};

class LfoBlock : public juce::Component
{
public:
    LfoBlock (juce::String title,
              juce::String freqLabel,
              juce::String depthLabel,
              const juce::StringArray& typeItems)
        : freq_ (std::move (freqLabel)),
          depth_ (std::move (depthLabel)),
          type_ ("Shape")
    {
        header_.setText (std::move (title), juce::dontSendNotification);
        header_.setFont (juce::Font (11.0f, juce::Font::bold));
        header_.setColour (juce::Label::textColourId, juce::Colour (PhazonColors::TextPrimary));
        header_.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (header_);

        for (const auto& item : typeItems)
            type_.combo().addItem (item, type_.combo().getNumItems() + 1);

        sync_.setButtonText ("SYNC");

        addAndMakeVisible (freq_);
        addAndMakeVisible (depth_);
        addAndMakeVisible (type_);
        addAndMakeVisible (sync_);
    }

    SliderControl& freq() noexcept { return freq_; }
    SliderControl& depth() noexcept { return depth_; }
    ChoiceControl& type() noexcept { return type_; }
    juce::ToggleButton& sync() noexcept { return sync_; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (PhazonColors::HeaderBG).withAlpha (0.45f));
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (juce::Colour (PhazonColors::PanelBorder).withAlpha (0.7f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10, 10);
        header_.setBounds (area.removeFromTop (18));
        area.removeFromTop (4);
        freq_.setBounds (area.removeFromTop (42));
        area.removeFromTop (4);
        depth_.setBounds (area.removeFromTop (42));
        area.removeFromTop (4);
        type_.setBounds (area.removeFromTop (42));
        area.removeFromTop (8);
        sync_.setBounds (area.removeFromTop (22));
    }

private:
    juce::Label header_;
    SliderControl freq_;
    SliderControl depth_;
    ChoiceControl type_;
    juce::ToggleButton sync_;
};

class ModSlotRow : public juce::Component
{
public:
    ModSlotRow() : source_ ("Source"), destination_ ("Destination"), amount_ ("Amount")
    {
        addAndMakeVisible (source_);
        addAndMakeVisible (destination_);
        addAndMakeVisible (amount_);
    }

    ChoiceControl& source() noexcept { return source_; }
    ChoiceControl& destination() noexcept { return destination_; }
    SliderControl& amount() noexcept { return amount_; }

    void resized() override
    {
        auto area = getLocalBounds();
        auto amountWidth = juce::jmin (280, area.getWidth() / 3);
        source_.setBounds (area.removeFromLeft ((area.getWidth() - amountWidth) / 2).reduced (2, 0));
        destination_.setBounds (area.removeFromLeft (area.getWidth() - amountWidth).reduced (2, 0));
        amount_.setBounds (area.reduced (2, 0));
    }

private:
    ChoiceControl source_;
    ChoiceControl destination_;
    SliderControl amount_;
};

class ModMatrixComponent : public juce::Component
{
public:
    ModMatrixComponent()
    {
        for (size_t i = 0; i < slots_.size(); ++i)
        {
            auto& slot = slots_[i];
            addAndMakeVisible (slot);

            for (const auto* label : ModMatrix::sourceLabels)
                slot.source().combo().addItem (label, slot.source().combo().getNumItems() + 1);

            for (const auto* label : ModMatrix::destinationLabels)
                slot.destination().combo().addItem (label, slot.destination().combo().getNumItems() + 1);
        }
    }

    ModSlotRow& slot (int index) noexcept { return slots_[(size_t) index]; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (PhazonColors::HeaderBG).withAlpha (0.30f));
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (juce::Colour (PhazonColors::PanelBorder).withAlpha (0.6f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8, 8);
        const int rowHeight = juce::jmax (38, area.getHeight() / (int) slots_.size());

        for (auto& slot : slots_)
            slot.setBounds (area.removeFromTop (rowHeight).reduced (0, 2));
    }

private:
    std::array<ModSlotRow, ModMatrix::kSlotCount> slots_;
};

void configureComboItems (juce::ComboBox& combo, const juce::StringArray& items)
{
    for (const auto& item : items)
        combo.addItem (item, combo.getNumItems() + 1);
}
} // namespace

class PhazonAudioProcessorEditor::Impl
{
public:
    Impl (PhazonAudioProcessorEditor& editor, PhazonAudioProcessor& processor)
        : editor_ (editor),
          processor_ (processor),
          presetBrowser_ (processor.presetManager),
          mainPanel_ ("Main"),
          modulationPanel_ ("Movement / Matrix"),
          filterPanel_ ("Filter + Space"),
          visualizerPanel_ ("Visualizer"),
          presetPanel_ ("Presets"),
          springDamping_ ("Spring Damping"),
          massDamping_ ("Mass Damping"),
          bowForce_ ("Bow Force"),
          excitationRatio_ ("Excitation Ratio"),
          nonlinearity_ ("Nonlinearity"),
          drive_ ("Drive"),
          chaos_ ("Chaos"),
          filterCutoff_ ("Cutoff"),
          filterResonance_ ("Resonance"),
          spaceMix_ ("Space Mix"),
          spaceSize_ ("Space Size"),
          outputGain_ ("Output"),
          excitationMode_ ("Excitation"),
          filterType_ ("Filter Type"),
          movement_ ("Movement", "Rate", "Depth", { "Sine", "Triangle", "Saw", "Square", "Random" }),
          modulation_ ("Modulation", "Rate", "Depth", { "Sine", "Triangle", "Saw", "Square", "Random" }),
          vibrato_ ("Vibrato", "Rate", "Depth", { "Sine", "Triangle", "Saw", "Square", "Random" }),
          matrixLfo_ ("Matrix LFO", "Rate", "Depth", { "Sine", "Square", "Random", "Triangle" })
    {
        editor_.setLookAndFeel (&lookAndFeel_);

        title_.setText ("PHAZON", juce::dontSendNotification);
        title_.setFont (juce::Font (21.0f, juce::Font::bold));
        title_.setColour (juce::Label::textColourId, juce::Colour (PhazonColors::TextPrimary));
        title_.setJustificationType (juce::Justification::centredLeft);

        subtitle_.setText ("M6  EDITOR  /  VISUALIZER", juce::dontSendNotification);
        subtitle_.setFont (juce::Font (10.0f));
        subtitle_.setColour (juce::Label::textColourId, juce::Colour (PhazonColors::TextDim));
        subtitle_.setJustificationType (juce::Justification::centredLeft);

        ecoToggle_.setButtonText ("ECO");
        monoToggle_.setButtonText ("MONO");

        configureComboItems (filterType_.combo(), { "LP", "HP", "BP", "Notch" });
        configureComboItems (excitationMode_.combo(), { "Bow", "Pluck", "Strike" });
        matrixLfo_.sync().setVisible (false);

        addToEditor (title_, subtitle_,
                     mainPanel_, modulationPanel_, filterPanel_, visualizerPanel_, presetPanel_,
                     excitationMode_, ecoToggle_, monoToggle_);

        addToPanel (mainPanel_, springDamping_, massDamping_, bowForce_, excitationRatio_,
                    nonlinearity_, drive_, chaos_);
        addToPanel (modulationPanel_, movement_, modulation_, vibrato_, matrixLfo_, modMatrix_);
        addToPanel (filterPanel_, filterCutoff_, filterResonance_, spaceMix_, spaceSize_,
                    outputGain_, filterType_);
        addToPanel (visualizerPanel_, visualizer_);
        addToPanel (presetPanel_, presetBrowser_);

        visualizer_.onFetchData = [this] (float* nodes, int& count, float& rms)
        {
            processor_.getVisualizerData (nodes, count, rms);
        };

        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::springDamping, springDamping_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::massDamping, massDamping_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::bowForce, bowForce_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::excitationRatio, excitationRatio_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::nonlinearity, nonlinearity_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::drive, drive_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::chaos, chaos_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::filterCutoff, filterCutoff_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::filterResonance, filterResonance_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::SpaceMix, spaceMix_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::SpaceSize, spaceSize_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::outputGain, outputGain_.slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::MovementFREQ, movement_.freq().slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::MovementDEPTH, movement_.depth().slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::ModulationFREQ, modulation_.freq().slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::ModulationDEPTH, modulation_.depth().slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::vibratoFreq, vibrato_.freq().slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::vibratoDepth, vibrato_.depth().slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::modLfoRate, matrixLfo_.freq().slider()));
        sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::modLfoDepth, matrixLfo_.depth().slider()));

        for (int i = 0; i < ModMatrix::kSlotCount; ++i)
            sliderAttachments_.push_back (std::make_unique<SliderAttachment> (processor_.apvts, ParamIDs::modSlotAmount[(size_t) i], modMatrix_.slot (i).amount().slider()));

        comboAttachments_.push_back (std::make_unique<ComboAttachment> (processor_.apvts, ParamIDs::filterType, filterType_.combo()));
        comboAttachments_.push_back (std::make_unique<ComboAttachment> (processor_.apvts, ParamIDs::excitationMode, excitationMode_.combo()));
        comboAttachments_.push_back (std::make_unique<ComboAttachment> (processor_.apvts, ParamIDs::MovementTYPE, movement_.type().combo()));
        comboAttachments_.push_back (std::make_unique<ComboAttachment> (processor_.apvts, ParamIDs::ModulationTYPE, modulation_.type().combo()));
        comboAttachments_.push_back (std::make_unique<ComboAttachment> (processor_.apvts, ParamIDs::VibratoTYPE, vibrato_.type().combo()));
        comboAttachments_.push_back (std::make_unique<ComboAttachment> (processor_.apvts, ParamIDs::modLfoShape, matrixLfo_.type().combo()));

        for (int i = 0; i < ModMatrix::kSlotCount; ++i)
        {
            comboAttachments_.push_back (std::make_unique<ComboAttachment> (processor_.apvts, ParamIDs::modSlotSource[(size_t) i], modMatrix_.slot (i).source().combo()));
            comboAttachments_.push_back (std::make_unique<ComboAttachment> (processor_.apvts, ParamIDs::modSlotDestination[(size_t) i], modMatrix_.slot (i).destination().combo()));
        }

        buttonAttachments_.push_back (std::make_unique<ButtonAttachment> (processor_.apvts, ParamIDs::ecoMode, ecoToggle_));
        buttonAttachments_.push_back (std::make_unique<ButtonAttachment> (processor_.apvts, ParamIDs::monoMode, monoToggle_));
        buttonAttachments_.push_back (std::make_unique<ButtonAttachment> (processor_.apvts, ParamIDs::MovementSYNC, movement_.sync()));
        buttonAttachments_.push_back (std::make_unique<ButtonAttachment> (processor_.apvts, ParamIDs::ModulationSYNC, modulation_.sync()));
        buttonAttachments_.push_back (std::make_unique<ButtonAttachment> (processor_.apvts, ParamIDs::vibratoSync, vibrato_.sync()));
    }

    ~Impl()
    {
        editor_.setLookAndFeel (nullptr);
    }

    void paint (juce::Graphics& g)
    {
        auto bounds = editor_.getLocalBounds().toFloat();

        juce::ColourGradient bg (juce::Colour (PhazonColors::BG).brighter (0.08f),
                                 0.0f, 0.0f,
                                 juce::Colour (PhazonColors::BG),
                                 bounds.getWidth(), bounds.getHeight(),
                                 false);
        bg.addColour (0.55, juce::Colour (0xff0a1119));
        g.setGradientFill (bg);
        g.fillAll();

        g.setColour (juce::Colour (PhazonColors::Accent).withAlpha (0.06f));
        g.fillEllipse (bounds.getWidth() - 320.0f, 36.0f, 240.0f, 240.0f);

        g.setColour (juce::Colour (PhazonColors::AccentWarm).withAlpha (0.035f));
        g.fillEllipse (38.0f, bounds.getHeight() - 240.0f, 280.0f, 180.0f);
    }

    void resized()
    {
        auto area = editor_.getLocalBounds().reduced (18);

        auto topBar = area.removeFromTop (48);
        auto titleArea = topBar.removeFromLeft (260);
        title_.setBounds (titleArea.removeFromTop (24));
        subtitle_.setBounds (titleArea.removeFromTop (16));

        auto rightBar = topBar.removeFromRight (350);
        monoToggle_.setBounds (rightBar.removeFromRight (84).reduced (4, 6));
        ecoToggle_.setBounds (rightBar.removeFromRight (72).reduced (4, 6));
        excitationMode_.setBounds (rightBar.removeFromRight (182));

        area.removeFromTop (12);

        const bool compact = area.getWidth() < 1120;

        if (compact)
        {
            auto upper = area.removeFromTop (area.proportionOfHeight (0.54f));
            auto lower = area;

            layoutLeftArea (upper);

            visualizerPanel_.setBounds (lower.removeFromTop (240));
            layoutVisualizerPanel();
            lower.removeFromTop (12);
            presetPanel_.setBounds (lower);
            layoutPresetPanel();
            return;
        }

        auto left = area.removeFromLeft ((int) (area.getWidth() * 0.63f));
        auto right = area;
        left.removeFromRight (10);
        right.removeFromLeft (10);

        layoutLeftArea (left);

        visualizerPanel_.setBounds (right.removeFromTop (250));
        layoutVisualizerPanel();
        right.removeFromTop (12);
        presetPanel_.setBounds (right);
        layoutPresetPanel();
    }

private:
    template <typename... Components>
    void addToEditor (Components&... components)
    {
        (editor_.addAndMakeVisible (components), ...);
    }

    template <typename... Components>
    void addToPanel (Panel& panel, Components&... components)
    {
        (panel.addAndMakeVisible (components), ...);
    }

    void layoutLeftArea (juce::Rectangle<int> area)
    {
        mainPanel_.setBounds (area.removeFromTop ((int) (area.getHeight() * 0.48f)));
        layoutMainPanel();

        area.removeFromTop (12);

        auto bottom = area;
        modulationPanel_.setBounds (bottom.removeFromLeft ((int) (bottom.getWidth() * 0.62f)));
        layoutModulationPanel();

        bottom.removeFromLeft (12);
        filterPanel_.setBounds (bottom);
        layoutFilterPanel();
    }

    void layoutMainPanel()
    {
        auto content = mainPanel_.getContentBounds();
        juce::Grid grid;
        grid.autoColumns = juce::Grid::TrackInfo (juce::Grid::Fr (1));
        grid.autoRows = juce::Grid::TrackInfo (juce::Grid::Fr (1));
        grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                                 juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                                 juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                                 juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
        grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                              juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
        grid.columnGap = juce::Grid::Px (8.0f);
        grid.rowGap = juce::Grid::Px (8.0f);
        grid.items = {
            juce::GridItem (springDamping_),
            juce::GridItem (massDamping_),
            juce::GridItem (bowForce_),
            juce::GridItem (excitationRatio_),
            juce::GridItem (nonlinearity_),
            juce::GridItem (drive_),
            juce::GridItem (chaos_)
        };
        grid.performLayout (content);
    }

    void layoutModulationPanel()
    {
        auto content = modulationPanel_.getContentBounds();
        auto top = content.removeFromTop (170);
        auto third = top.getWidth() / 3;
        movement_.setBounds (top.removeFromLeft (third).reduced (0, 2));
        modulation_.setBounds (top.removeFromLeft (third).reduced (4, 2));
        vibrato_.setBounds (top.reduced (4, 2));

        content.removeFromTop (10);
        auto bottom = content;
        matrixLfo_.setBounds (bottom.removeFromLeft (180).reduced (0, 2));
        bottom.removeFromLeft (10);
        modMatrix_.setBounds (bottom);
    }

    void layoutFilterPanel()
    {
        auto content = filterPanel_.getContentBounds();
        auto top = content.removeFromTop (content.getHeight() / 2);

        juce::Grid topGrid;
        topGrid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                                    juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                                    juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
        topGrid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
        topGrid.columnGap = juce::Grid::Px (8.0f);
        topGrid.items = {
            juce::GridItem (filterCutoff_),
            juce::GridItem (filterResonance_),
            juce::GridItem (outputGain_)
        };
        topGrid.performLayout (top);

        content.removeFromTop (8);
        auto choiceBounds = content.removeFromTop (42);
        filterType_.setBounds (choiceBounds);
        content.removeFromTop (8);

        juce::Grid bottomGrid;
        bottomGrid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                                       juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
        bottomGrid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
        bottomGrid.columnGap = juce::Grid::Px (8.0f);
        bottomGrid.items = {
            juce::GridItem (spaceMix_),
            juce::GridItem (spaceSize_)
        };
        bottomGrid.performLayout (content);
    }

    void layoutVisualizerPanel()
    {
        auto content = visualizerPanel_.getContentBounds();
        visualizer_.setBounds (content);
    }

    void layoutPresetPanel()
    {
        auto content = presetPanel_.getContentBounds();
        presetBrowser_.setBounds (content);
    }

    PhazonAudioProcessorEditor& editor_;
    PhazonAudioProcessor& processor_;
    PhazonLookAndFeel lookAndFeel_;

    juce::Label title_;
    juce::Label subtitle_;

    Panel mainPanel_;
    Panel modulationPanel_;
    Panel filterPanel_;
    Panel visualizerPanel_;
    Panel presetPanel_;

    KnobControl springDamping_;
    KnobControl massDamping_;
    KnobControl bowForce_;
    KnobControl excitationRatio_;
    KnobControl nonlinearity_;
    KnobControl drive_;
    KnobControl chaos_;

    KnobControl filterCutoff_;
    KnobControl filterResonance_;
    KnobControl spaceMix_;
    KnobControl spaceSize_;
    KnobControl outputGain_;

    ChoiceControl excitationMode_;
    ChoiceControl filterType_;
    juce::ToggleButton ecoToggle_;
    juce::ToggleButton monoToggle_;

    LfoBlock movement_;
    LfoBlock modulation_;
    LfoBlock vibrato_;
    LfoBlock matrixLfo_;
    ModMatrixComponent modMatrix_;

    StringVisualizer visualizer_;
    PresetBrowser presetBrowser_;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments_;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments_;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments_;
};

PhazonAudioProcessorEditor::PhazonAudioProcessorEditor (PhazonAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), impl_ (std::make_unique<Impl> (*this, p))
{
    setSize (1280, 820);
}

PhazonAudioProcessorEditor::~PhazonAudioProcessorEditor() = default;

void PhazonAudioProcessorEditor::paint (juce::Graphics& g)
{
    impl_->paint (g);
}

void PhazonAudioProcessorEditor::resized()
{
    impl_->resized();
}
