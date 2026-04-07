#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "presets/PresetManager.h"
#include "PhazonLookAndFeel.h"

// =============================================================================
// PresetBrowser
//
// Factory presets: ComboBox (load on change)
// User presets:    ListBox  (double-click or LOAD button)
// Save:            TextEditor + SAVE button
// Delete:          DEL button
// =============================================================================
class PresetBrowser : public juce::Component,
                      private juce::ListBoxModel
{
public:
    explicit PresetBrowser (PresetManager& pm) : pm_ (pm)
    {
        // Section headers
        auto initLabel = [this] (juce::Label& l, const juce::String& text)
        {
            l.setText (text, juce::dontSendNotification);
            l.setFont (juce::Font (9.5f, juce::Font::bold));
            l.setJustificationType (juce::Justification::centredLeft);
            l.setColour (juce::Label::textColourId, juce::Colour (PhazonColors::TextDim));
            addAndMakeVisible (l);
        };

        initLabel (factoryLabel_, "FACTORY");
        initLabel (userLabel_,    "USER");

        // Factory combo
        addAndMakeVisible (factoryCombo_);
        for (auto& name : pm_.getFactoryPresetNames())
            factoryCombo_.addItem (name, factoryCombo_.getNumItems() + 1);
        if (factoryCombo_.getNumItems() > 0)
            factoryCombo_.setSelectedId (1, juce::dontSendNotification);
        factoryCombo_.onChange = [this] { loadFactory(); };

        // User list
        addAndMakeVisible (userList_);
        userList_.setModel (this);
        userList_.setRowHeight (20);
        userList_.setMultipleSelectionEnabled (false);
        userList_.setColour (juce::ListBox::outlineColourId,
                             juce::Colour (PhazonColors::PanelBorder));
        userList_.setOutlineThickness (1);

        // Save name editor
        addAndMakeVisible (saveEditor_);
        saveEditor_.setTextToShowWhenEmpty ("preset name...",
                                             juce::Colour (PhazonColors::TextDim));
        saveEditor_.setFont (juce::Font (11.0f));
        saveEditor_.onReturnKey = [this] { saveUser(); };

        // Buttons
        auto initBtn = [this] (juce::TextButton& b, const juce::String& text)
        {
            b.setButtonText (text);
            addAndMakeVisible (b);
        };

        initBtn (saveBtn_,   "SAVE");
        initBtn (loadBtn_,   "LOAD");
        initBtn (deleteBtn_, "DEL");

        saveBtn_  .onClick = [this] { saveUser();   };
        loadBtn_  .onClick = [this] { loadUser();   };
        deleteBtn_.onClick = [this] { deleteUser(); };

        refreshUserList();
    }

    // -------------------------------------------------------------------------
    void resized() override
    {
        auto area = getLocalBounds().reduced (6, 6);
        const int lh  = 14;
        const int row = 22;
        const int gap = 4;

        factoryLabel_.setBounds (area.removeFromTop (lh));
        area.removeFromTop (2);
        factoryCombo_.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap * 2);

        userLabel_.setBounds (area.removeFromTop (lh));
        area.removeFromTop (2);

        // Reserve space at bottom for editor + buttons
        const int bottomH = row + gap + row;
        userList_.setBounds (area.removeFromTop (area.getHeight() - bottomH - gap * 2));

        area.removeFromTop (gap);
        saveEditor_.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);

        auto btnRow = area.removeFromTop (row);
        const int bw = btnRow.getWidth() / 3;
        saveBtn_  .setBounds (btnRow.removeFromLeft (bw).reduced (2, 1));
        loadBtn_  .setBounds (btnRow.removeFromLeft (bw).reduced (2, 1));
        deleteBtn_.setBounds (btnRow.reduced (2, 1));
    }

    // -------------------------------------------------------------------------
    void refreshUserList()
    {
        userNames_ = pm_.getUserPresetNames();
        userList_.updateContent();
        userList_.repaint();
    }

private:
    PresetManager&    pm_;
    juce::StringArray userNames_;

    juce::Label       factoryLabel_, userLabel_;
    juce::ComboBox    factoryCombo_;
    juce::ListBox     userList_;
    juce::TextEditor  saveEditor_;
    juce::TextButton  saveBtn_, loadBtn_, deleteBtn_;

    // --- ListBoxModel --------------------------------------------------------
    int getNumRows() override
    {
        return userNames_.size();
    }

    void paintListBoxItem (int row, juce::Graphics& g,
                           int w, int h, bool selected) override
    {
        if (! juce::isPositiveAndBelow (row, userNames_.size()))
            return;

        if (selected)
        {
            g.setColour (juce::Colour (PhazonColors::BtnActive));
            g.fillRect (0, 0, w, h);
        }

        g.setColour (selected ? juce::Colour (PhazonColors::Accent)
                              : juce::Colour (PhazonColors::TextPrimary));
        g.setFont (juce::Font (10.5f));
        g.drawText (userNames_[row], 8, 0, w - 8, h,
                    juce::Justification::centredLeft, true);
    }

    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
    {
        if (juce::isPositiveAndBelow (row, userNames_.size()))
            pm_.loadUserPreset (userNames_[row]);
    }

    // --- Actions -------------------------------------------------------------
    void loadFactory()
    {
        const int idx = factoryCombo_.getSelectedId() - 1;
        if (idx >= 0) pm_.loadFactoryPreset (idx);
    }

    void loadUser()
    {
        const int row = userList_.getSelectedRow();
        if (juce::isPositiveAndBelow (row, userNames_.size()))
            pm_.loadUserPreset (userNames_[row]);
    }

    void saveUser()
    {
        const auto name = saveEditor_.getText().trim();
        if (name.isNotEmpty())
        {
            pm_.saveUserPreset (name);
            saveEditor_.clear();
            refreshUserList();
        }
    }

    void deleteUser()
    {
        const int row = userList_.getSelectedRow();
        if (juce::isPositiveAndBelow (row, userNames_.size()))
        {
            pm_.deleteUserPreset (userNames_[row]);
            refreshUserList();
        }
    }
};
