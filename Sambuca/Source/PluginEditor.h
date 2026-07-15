#pragma once 
#include <juce_audio_processors/juce_audio_processors.h> 
#include "PluginProcessor.h"

// ==============================================================================
// 1. LOOKANDFEEL PERSONALIZZATO (PNG SKIN)
// ==============================================================================
class PngLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PngLookAndFeel() {}

    void setImages (juce::Image bg, juce::Image kOff, juce::Image kBase, juce::Image kGlow, 
                    juce::Image kSteps, juce::Image sHandle, juce::Image bNormal, juce::Image bPressed)
    {
        bgImg = bg;
        knobOffImg = kOff;
        knobBaseImg = kBase;
        knobGlowImg = kGlow;
        knobStepsImg = kSteps;
        sliderHandleImg = sHandle;
        btnNormalImg = bNormal;
        btnPressedImg = bPressed;
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        if (knobBaseImg.isValid())
        {
            auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
            auto minDim = std::min(bounds.getWidth(), bounds.getHeight());
            auto knobRect = juce::Rectangle<float>(0, 0, minDim, minDim).withCentre(bounds.getCentre());

            g.drawImageWithin(knobBaseImg, knobRect.getX(), knobRect.getY(), 
                              knobRect.getWidth(), knobRect.getHeight(), 
                              juce::RectanglePlacement::centred);

            float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

            if (knobGlowImg.isValid())
            {
                juce::AffineTransform rx = juce::AffineTransform::rotation (angle, bounds.getCentreX(), bounds.getCentreY());
                g.drawImageTransformed (knobGlowImg, rx);
            }
        }
        else
        {
            juce::LookAndFeel_V4::drawRotarySlider (g, x, y, width, height, sliderPosProportional, 
                                                    rotaryStartAngle, rotaryEndAngle, slider);
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        if (shouldDrawButtonAsDown && btnPressedImg.isValid())
        {
            g.drawImageWithin (btnPressedImg, 0, 0, button.getWidth(), button.getHeight(), juce::RectanglePlacement::stretchToFit);
        }
        else if (btnNormalImg.isValid())
        {
            g.drawImageWithin (btnNormalImg, 0, 0, button.getWidth(), button.getHeight(), juce::RectanglePlacement::stretchToFit);
        }
        else
        {
            juce::LookAndFeel_V4::drawButtonBackground (g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }

    juce::Image bgImg, knobOffImg, knobBaseImg, knobGlowImg, knobStepsImg, sliderHandleImg, btnNormalImg, btnPressedImg;
};

// ==============================================================================
// 2. COMPONENTE OSCILLOSCOPIO GRAFICO A 30 FPS 
// ==============================================================================
class OscilloscopeComponent : public juce::Component, public juce::Timer 
{ 
public: 
    OscilloscopeComponent (SambucaAudioProcessor& p) : processor (p) 
    { 
        startTimerHz (30); 
    } 
    
    ~OscilloscopeComponent() override 
    {      
        stopTimer(); 
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xFF11101A));
        g.setColour (juce::Colour (0xFF35324A));
        g.drawRect (getLocalBounds(), 2);
        g.setColour (juce::Colours::orange);

        juce::Path wavePath;
        auto width  = (float) getWidth();
        auto height = (float) getHeight();
        auto midY   = height / 2.0f;
        wavePath.startNewSubPath (0.0f, midY);

        int totalSamples = SambucaAudioProcessor::fftSize;
        float scaleX = width / (float) totalSamples;

        for (int i = 0; i < totalSamples; ++i)
        {
            float sample = processor.visualizerBuffer[i] * 0.9f;
            float x = (float) i * scaleX;
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
    juce::TextButton loadSample1Btn;
juce::TextButton loadSample2Btn;
juce::TextButton loadSample3Btn;

void setupSampleButton (juce::TextButton& b, int oscIndex);
};

// ==============================================================================
// 3. EDITOR PRINCIPALE A PAGINA UNICA
// ==============================================================================
class SambucaAudioProcessorEditor : public juce::AudioProcessorEditor 
{ 
public: 
    SambucaAudioProcessorEditor (SambucaAudioProcessor&); 
    ~SambucaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Metodi per la gestione del caricamento campioni/skin
    void loadSampleFile();
    void loadPngFolder();

private: 
    SambucaAudioProcessor& audioProcessor;

    OscilloscopeComponent oscilloscope;
    juce::Rectangle<int> logoArea;

    juce::Label secOscLabel, secFilterLabel, secEnvLabel, secLfoLabel, secFxLabel;

    // OSCILLATORI
    juce::Slider   v1Slider, v2Slider, v3Slider, p1Slider, p2Slider, p3Slider, morphSlider;
    juce::ComboBox w1Combo, w2Combo, w3Combo;
    juce::Label    oscL1, oscL2, oscL3, oscL4, oscL5, oscL6, oscL7, oscL8, oscL9, oscL10;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   v1Att, v2Att, v3Att, p1Att, p2Att, p3Att, morphAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> w1Att, w2Att, w3Att;

    // FILTRI
    juce::Slider   cut1Slider, res1Slider, drive1Slider, cut2Slider, res2Slider, drive2Slider;
    juce::ComboBox type1Combo, type2Combo;
    juce::Label    fltL1, fltL2, fltL3, fltL4, fltL5, fltL6, fltL7, fltL8;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   cut1Att, res1Att, drive1Att, cut2Att, res2Att, drive2Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> type1Att, type2Att;

    // ENVELOPE
    juce::Slider attSlider, decSlider, susSlider, relSlider, scaleSlider;
    juce::Label  envL1, envL2, envL3, envL4, envL5;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attAtt, decAtt, susAtt, relAtt, scaleAtt;

    // LFO
    juce::Slider   rate1Slider, amt1Slider, rate2Slider, amt2Slider, rate3Slider, amt3Slider;
    juce::ComboBox wave1Combo, tgt1Combo, wave2Combo, tgt2Combo, wave3Combo, tgt3Combo;
    juce::Label    lfoL1, lfoL2, lfoL3, lfoL4, lfoL5, lfoL6, lfoL7, lfoL8, lfoL9, lfoL10, lfoL11, lfoL12;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   rate1Att, amt1Att, rate2Att, amt2Att, rate3Att, amt3Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> wave1Att, tgt1Att, wave2Att, tgt2Att, wave3Att, tgt3Att;

    // FX & MASTER
    juce::Slider delayTimeSlider, delayFbSlider, reverbSizeSlider, fxMixSlider, masterVolSlider;
    juce::Label  fxL1, fxL2, fxL3, fxL4, fxL5;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dTimeAtt, dFbAtt, revAtt, mixAtt, volAtt;

    // CONTROLLI SOSTITUTIVI DEL PAD XY (DUE SLIDER VICINI)
    juce::Slider padXSlider, padYSlider;
    juce::Label  padXLabel, padYLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> padXAtt, padYAtt;

    void setupRotary (juce::Slider& s, juce::Label& l, const juce::String& text);
    void setupCombo  (juce::ComboBox& c, const juce::StringArray& items, juce::Label& l, const juce::String& text);

    // TASTI CARICAMENTO SAMPLE E SKIN
    juce::TextButton loadSampleButton;
    juce::TextButton loadSkinButton;
    
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessorEditor)
};
