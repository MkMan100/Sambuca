#include "PluginProcessor.h"
#include "PluginEditor.h"

SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), oscilloscope (p)
{
    // Finestra più larga per contenere ordinatamente tutti i componenti
    setSize (800, 450);

    // Oscilloscopio
    addAndMakeVisible (oscilloscope);

    // --- Configurazione Sliders ---
    addAndMakeVisible (wavetableMorphSlider);
    wavetableMorphSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    wavetableMorphSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "wavetableMorph", wavetableMorphSlider);

    addAndMakeVisible (filterCutoffSlider);
    filterCutoffSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    filterCutoffSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "filter1Cutoff", filterCutoffSlider);

    addAndMakeVisible (lfoRateSlider);
    lfoRateSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lfoRateSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    lfoRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "lfo1Rate", lfoRateSlider);

    addAndMakeVisible (lfoAmountSlider);
    lfoAmountSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lfoAmountSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    lfoAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "lfo1Amount", lfoAmountSlider);

    // --- Configurazione Selettore Target LFO ---
    addAndMakeVisible (lfoTargetComboBox);
    lfoTargetComboBox.addItem ("None", 1);
    lfoTargetComboBox.addItem ("Filter Cutoff", 2);
    lfoTargetComboBox.addItem ("Osc Volume", 3);
    lfoTargetComboBox.addItem ("Osc Pitch", 4);
    lfoTargetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "lfo1Target", lfoTargetComboBox);

    // --- Etichette di testo ---
    addAndMakeVisible (morphLabel);
    morphLabel.setText ("Wavetable Morph", juce::dontSendNotification);
    morphLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (cutoffLabel);
    cutoffLabel.setText ("Filter Cutoff", juce::dontSendNotification);
    cutoffLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (lfoRateLabel);
    lfoRateLabel.setText ("LFO Rate", juce::dontSendNotification);
    lfoRateLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (lfoAmountLabel);
    lfoAmountLabel.setText ("LFO Amount", juce::dontSendNotification);
    lfoAmountLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (lfoTargetLabel);
    lfoTargetLabel.setText ("LFO Target", juce::dontSendNotification);
    lfoTargetLabel.setJustificationType (juce::Justification::centred);
}

SambucaAudioProcessorEditor::~SambucaAudioProcessorEditor() {}

void SambucaAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF1A1926));
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawText ("SAMBUCA SYNTH - Dashboard", getLocalBounds().removeFromTop (30), juce::Justification::centred, true);
}

void SambucaAudioProcessorEditor::resized()
{
    // Disposizione pulita, a griglia, per evitare qualunque tipo di sovrapposizione
    auto area = getLocalBounds().reduced (15);
    area.removeFromTop (30); // Spazio per il titolo

    // Metà superiore (Oscilloscopio)
    auto topArea = area.removeFromTop (180);
    oscilloscope.setBounds (topArea);

    area.removeFromTop (15); // Divisorio

    // Metà inferiore (Controlli distribuiti uniformemente su 5 colonne)
    auto bottomArea = area;
    int colWidth = bottomArea.getWidth() / 5;

    auto col1 = bottomArea.removeFromLeft (colWidth);
    morphLabel.setBounds (col1.removeFromTop (20));
    wavetableMorphSlider.setBounds (col1.reduced (10));

    auto col2 = bottomArea.removeFromLeft (colWidth);
    cutoffLabel.setBounds (col2.removeFromTop (20));
    filterCutoffSlider.setBounds (col2.reduced (10));

    auto col3 = bottomArea.removeFromLeft (colWidth);
    lfoRateLabel.setBounds (col3.removeFromTop (20));
    lfoRateSlider.setBounds (col3.reduced (10));

    auto col4 = bottomArea.removeFromLeft (colWidth);
    lfoAmountLabel.setBounds (col4.removeFromTop (20));
    lfoAmountSlider.setBounds (col4.reduced (10));

    auto col5 = bottomArea;
    lfoTargetLabel.setBounds (col5.removeFromTop (20));
    // Centra il ComboBox verticalmente nella sua colonna
    lfoTargetComboBox.setBounds (col5.withSizeKeepingCentre (colWidth - 20, 30));
}
