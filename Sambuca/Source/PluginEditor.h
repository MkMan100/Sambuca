#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

// ==============================================================================
class PngLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PngLookAndFeel() {}
    void setImages (juce::Image bg, juce::Image, juce::Image, juce::Image, 
                    juce::Image, juce::Image, juce::Image, juce::Image)
    {
        background = bg;
    }
private:
    juce::Image background;
};

// ==============================================================================
class OscilloscopeComponent : public juce::Component
{
public:
    OscilloscopeComponent (SambucaAudioProcessor& p) : processor (p) {}
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black.withAlpha (0.4f)); }
private:
    SambucaAudioProcessor& processor;
};

// ==============================================================================
class SambucaAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SambucaAudioProcessorEditor (SambucaAudioProcessor&);
    ~SambucaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setupRotary (juce::Slider& s, juce::Label& l, const juce::String& text);
    void setupCombo (juce::ComboBox& c, const juce::StringArray& items, juce::Label& l, const juce::String& text);
    void setupSampleButton (juce::TextButton& b, int oscIndex);

    void toggleSkinMode();
    void loadPngFolder();

private:
    SambucaAudioProcessor& audioProcessor;
    OscilloscopeComponent oscilloscope;

    // Pulsanti caricamento campioni
    juce::TextButton loadSample1Btn;
    juce::TextButton loadSample2Btn;
    juce::TextButton loadSample3Btn;

    // Skin Custom
    juce::TextButton skinModeButton;
    juce::TextButton loadFolderButton;
    bool useCustomSkin = false;
    juce::Image loadedBg;
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Rectangle<int> logoArea;

    // Etichette Sezioni
    juce::Label secOscLabel, secFilterLabel, secEnvLabel, secLfoLabel, secFxLabel;

    // Controlli OSC 1, 2, 3
    juce::Slider v1Slider, p1Slider, v2Slider, p2Slider, v3Slider, p3Slider, morphSlider;
    juce::ComboBox w1Combo, w2Combo, w3Combo;
    juce::Label oscL1, oscL2, oscL3, oscL4, oscL5, oscL6, oscL7, oscL8, oscL9, oscL10;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> v1Att, p1Att, v2Att, p2Att, v3Att, p3Att, morphAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> w1Att, w2Att, w3Att;

    // Filtri
    juce::Slider cut1Slider, res1Slider, drive1Slider, cut2Slider, res2Slider, drive2Slider;
    juce::ComboBox type1Combo, type2Combo;
    juce::Label fltL1, fltL2, fltL3, fltL4, fltL5, fltL6, fltL7, fltL8;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cut1Att, res1Att, drive1Att, cut2Att, res2Att, drive2Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> type1Att, type2Att;

    // ADSR Envelope
    juce::Slider attSlider, decSlider, susSlider, relSlider, scaleSlider;
    juce::Label envL1, envL2, envL3, envL4, envL5;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attAtt, decAtt, susAtt, relAtt, scaleAtt;

    // LFO 1, 2, 3
    juce::Slider rate1Slider, amt1Slider, rate2Slider, amt2Slider, rate3Slider, amt3Slider;
    juce::ComboBox wave1Combo, tgt1Combo, wave2Combo, tgt2Combo, wave3Combo, tgt3Combo;
    juce::Label lfoL1, lfoL2, lfoL3, lfoL4, lfoL5, lfoL6, lfoL7, lfoL8, lfoL9, lfoL10, lfoL11, lfoL12;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rate1Att, amt1Att, rate2Att, amt2Att, rate3Att, amt3Att;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> wave1Att, tgt1Att, wave2Att, tgt2Att, wave3Att, tgt3Att;

    // FX & MASTER
    juce::Slider delayTimeSlider, delayFbSlider, reverbSizeSlider, fxMixSlider, masterVolSlider;
    juce::Label fxL1, fxL2, fxL3, fxL4, fxL5;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dTimeAtt, dFbAtt, revAtt, mixAtt, volAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessorEditor)
};
