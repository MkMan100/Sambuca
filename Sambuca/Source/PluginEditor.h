#pragma once

#include <JuceHeader.h>
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

    // Struttura di supporto per raggruppare uno Slider e il suo Attachment automatico ad APVTS
    struct ConnectedSlider
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String section; // Identifica a quale macro-area appartiene il controllo
    };

    std::vector<ConnectedSlider> connectedSliders;
    juce::Image backgroundImage;

    // Helper per creare e collegare al volo un knob generico
    void createAndConnectKnob (const juce::String& parameterID, const juce::String& sectionName);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessorEditor)
};