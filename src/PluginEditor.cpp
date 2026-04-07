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
}

void PhazonAudioProcessorEditor::resized() {}
