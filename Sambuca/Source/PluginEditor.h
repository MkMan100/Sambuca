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

    // Struttura di supporto per raggruppare uno Slider e il suo Attachment
    struct ConnectedSlider
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String section; // Identifica a quale macro-area appartiene il controllo
    };

    // Modificato in un vettore di unique_ptr per evitare riallocazioni di memoria distruttive
    std::vector<std::unique_ptr<ConnectedSlider>> connectedSliders;
    juce::Image backgroundImage;

    // Helper per creare e collegare al volo un knob generico
    void createAndConnectKnob (const juce::String& parameterID, const juce::String& sectionName);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessorEditor)
};
