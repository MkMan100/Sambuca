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

// --- Editor principale con tab per tutti i controlli ---
class SambucaAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SambucaAudioProcessorEditor (SambucaAudioProcessor&);
    ~SambucaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SambucaAudioProcessor& audioProcessor;

    // Oscilloscopio fisso in alto
    OscilloscopeComponent oscilloscope;

    // Componente Tab per dividere ordinatamente i gruppi di parametri
    juce::TabbedComponent tabbedComponent { juce::TabbedButtonBar::TabsAtTop };

    // Sottocomponenti per ogni Tab
    class OscTab : public juce::Component
    {
    public:
        OscTab (SambucaAudioProcessor& p, SambucaAudioProcessorEditor& editor);
        void resized() override;
        
        juce::Slider v1Slider, v2Slider, v3Slider, p1Slider, p2Slider, p3Slider, morphSlider;
        juce::ComboBox w1Combo, w2Combo, w3Combo;
        juce::Label l1, l2, l3, l4, l5, l6, l7, l8, l9, l10;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> v1Att, v2Att, v3Att, p1Att, p2Att, p3Att, morphAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> w1Att, w2Att, w3Att;
    };

    class FilterTab : public juce::Component
    {
    public:
        FilterTab (SambucaAudioProcessor& p);
        void resized() override;

        juce::Slider cut1Slider, res1Slider, drive1Slider, cut2Slider, res2Slider, drive2Slider;
        juce::ComboBox type1Combo, type2Combo;
        juce::Label l1, l2, l3, l4, l5, l6, l7, l8;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cut1Att, res1Att, drive1Att, cut2Att, res2Att, drive2Att;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> type1Att, type2Att;
    };

    class EnvelopeTab : public juce::Component
    {
    public:
        EnvelopeTab (SambucaAudioProcessor& p);
        void resized() override;

        juce::Slider attSlider, decSlider, susSlider, relSlider, scaleSlider;
        juce::Label l1, l2, l3, l4, l5;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attAtt, decAtt, susAtt, relAtt, scaleAtt;
    };

    class LfoTab : public juce::Component
    {
    public:
        LfoTab (SambucaAudioProcessor& p);
        void resized() override;

        juce::Slider rate1Slider, amt1Slider, rate2Slider, amt2Slider, rate3Slider, amt3Slider;
        juce::ComboBox wave1Combo, tgt1Combo, wave2Combo, tgt2Combo, wave3Combo, tgt3Combo;
        juce::Label l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rate1Att, amt1Att, rate2Att, amt2Att, rate3Att, amt3Att;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> wave1Att, tgt1Att, wave2Att, tgt2Att, wave3Att, tgt3Att;
    };

    class FxMasterTab : public juce::Component
    {
    public:
        FxMasterTab (SambucaAudioProcessor& p);
        void resized() override;

        juce::Slider delayTimeSlider, delayFbSlider, reverbSizeSlider, fxMixSlider, masterVolSlider;
        juce::Label l1, l2, l3, l4, l5;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dTimeAtt, dFbAtt, revAtt, mixAtt, volAtt;
    };

    // Istante dei tab pannelli
    std::unique_ptr<OscTab> oscPanel;
    std::unique_ptr<FilterTab> filterPanel;
    std::unique_ptr<EnvelopeTab> envelopePanel;
    std::unique_ptr<LfoTab> lfoPanel;
    std::unique_ptr<FxMasterTab> fxMasterPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessorEditor)
};
