#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

class SambucaAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SambucaAudioProcessorEditor (SambucaAudioProcessor&);
    ~SambucaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SambucaAudioProcessor& audioProcessor;
    SambucaLookAndFeel sambucaLookAndFeel;

    struct ConnectedSlider
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        std::unique_ptr<juce::Label> label; // <-- AGGIUNTO PER I NOMI
        juce::String section; 
        juce::String cleanName;             // <-- AGGIUNTO PER IL TESTO PULITO
    };

    std::vector<std::unique_ptr<ConnectedSlider>> connectedSliders;
    juce::Image backgroundImage;

    // Pulsanti per il caricamento dei file WAV per i 3 oscillatori
    std::unique_ptr<juce::TextButton> loadButtonOsc1;
    std::unique_ptr<juce::TextButton> loadButtonOsc2;
    std::unique_ptr<juce::TextButton> loadButtonOsc3;

    void createAndConnectKnob (const juce::String& parameterID, const juce::String& sectionName, const juce::String& displayName);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessorEditor)
};
