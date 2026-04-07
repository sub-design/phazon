#include "PluginEditor.h"

PhazonAudioProcessorEditor::PhazonAudioProcessorEditor (PhazonAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize (800, 500);
}

PhazonAudioProcessorEditor::~PhazonAudioProcessorEditor() {}

void PhazonAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));
    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("Phazon", getLocalBounds(), juce::Justification::centred, 1);
}

void PhazonAudioProcessorEditor::resized()
{
    // TODO CN-289: lay out GUI components (ControlPanel, Crosshair, LFOsliders)
}
