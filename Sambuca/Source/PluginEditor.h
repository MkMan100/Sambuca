#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// --- Componente Oscilloscopio Grafico a 30 FPS ---
class OscilloscopeComponent : public juce::Component, public juce::Timer
{
public:
    OscilloscopeComponent (SambucaAudioProcessor& p) : processor (p) { startTimerHz (30); }
    ~OscilloscopeComponent() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xFF11101A));
        g.setColour (juce::Colour (0xFF35324A));
        g.drawRect (getLocalBounds(), 2);

        g.setColour (juce::Colours::orange);
        juce::Path wavePath;

        auto width = (float)getWidth();
        auto height = (float)getHeight();
        auto midY = height / 2.0f;

        wavePath.startNewSubPath (0.0f, midY);

        int totalSamples = SambucaAudioProcessor::fftSize;
        float scaleX = width / (float)totalSamples;
        
        for (int i = 0; i < totalSamples; ++i)
        {
            float sample = processor.visualizerBuffer[i] * 0.9f; 
            float x = (float)i * scaleX;
            float y = midY - (sample * midY);
            wavePath.lineTo (x, juce::jlimit (0.0f, height, y));
        }
        g.strokePath (wavePath, juce::PathStrokeType (1.5f));
    }

    void timerCallback() override
    {
        if (processor.hasNewSourceData)
        {
            processor.hasNewSourceData = false;
            repaint();
        }
    }

private:
    SambucaAudioProcessor& processor;
};

// --- Editor principale ---
class SambucaAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SambucaAudioProcessorEditor (SambucaAudioProcessor&);
    ~SambucaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SambucaAudioProcessor& audioProcessor;

    // Oscilloscopio
    OscilloscopeComponent oscilloscope;

    // Sliders
    juce::Slider wavetableMorphSlider;
    juce::Slider filterCutoffSlider;
    juce::Slider lfoRateSlider;
    juce::Slider lfoAmountSlider;

    // Etichette di Testo
    juce::Label morphLabel;
    juce::Label cutoffLabel;
    juce::Label lfoRateLabel;
    juce::Label lfoAmountLabel;
    juce::Label lfoTargetLabel;

    // Selettore Target LFO
    juce::ComboBox lfoTargetComboBox;

    // Allegati APVTS per sincronizzazione stabile
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoTargetAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessorEditor)
};
