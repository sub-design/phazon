#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class PhazonAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PhazonAudioProcessorEditor (PhazonAudioProcessor&);
    ~PhazonAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PhazonAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhazonAudioProcessorEditor)
};
