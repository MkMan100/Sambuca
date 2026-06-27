#include "PluginProcessor.h"
#include "PluginEditor.h"

SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. DIMENSIONI FINESTRA
    setSize (1024, 600);
    backgroundImage = juce::Image(); 

    // 2. CREAZIONE CONTROLLI CON NOMI PULITI
    // Oscillatori (OSC 1, 2, 3)
    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "osc" + juce::String(i);
        juce::String oscName = "OSC " + juce::String(i) + " ";
        createAndConnectKnob (prefix + "Waveform", "OSC", oscName + "Wave");
        createAndConnectKnob (prefix + "Volume", "OSC", oscName + "Vol");
        createAndConnectKnob (prefix + "Pitch", "OSC", oscName + "Pitch");
        createAndConnectKnob (prefix + "StartLoop", "OSC", oscName + "Loop");
    }

    // Filtri (Filter 1 & 2)
    for (int i = 1; i <= 2; ++i)
    {
        juce::String prefix = "filter" + juce::String(i);
        juce::String fltName = "FLT " + juce::String(i) + " ";
        createAndConnectKnob (prefix + "Type", "FILTER", fltName + "Type");
        createAndConnectKnob (prefix + "Cutoff", "FILTER", fltName + "Cut");
        createAndConnectKnob (prefix + "Resonance", "FILTER", fltName + "Res");
        createAndConnectKnob (prefix + "Drive", "FILTER", fltName + "Drive");
    }

    // LFOs (LFO 1, 2, 3)
    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "lfo" + juce::String(i);
        juce::String lfoName = "LFO " + juce::String(i) + " ";
        createAndConnectKnob (prefix + "Rate", "LFO", lfoName + "Rate");
        createAndConnectKnob (prefix + "Waveform", "LFO", lfoName + "Wave");
        createAndConnectKnob (prefix + "Amount", "LFO", lfoName + "Amt");
    }

    // Effetti
    createAndConnectKnob ("delayTime", "FX", "Delay Time");
    createAndConnectKnob ("delayFeedback", "FX", "Delay Fback");
    createAndConnectKnob ("reverbSize", "FX", "Reverb Size");
    createAndConnectKnob ("fxMix", "FX", "FX Mix");

    // Globali
    createAndConnectKnob ("wavetableMorph", "GLOBAL_SLIDER", "Wave Morph"); // Diventerà uno slider lineare
    createAndConnectKnob ("masterVolume", "GLOBAL", "Master Vol");
    createAndConnectKnob ("envTimeScale", "GLOBAL", "Env Scale");

// 3. INIZIALIZZAZIONE PULSANTI LOAD WAV
    loadButtonOsc1 = std::make_unique<juce::TextButton> ("Load OSC 1");
    loadButtonOsc2 = std::make_unique<juce::TextButton> ("Load OSC 2");
    loadButtonOsc3 = std::make_unique<juce::TextButton> ("Load OSC 3");

    auto setupButton = [this](juce::TextButton& btn, int oscNum) {
        btn.setButtonText ("LOAD WAV " + juce::String(oscNum));
        addAndMakeVisible (btn);
        
        btn.onClick = [this, oscNum]() {
            fileChooser = std::make_unique<juce::FileChooser> (
                "Seleziona un file WAV per l'Oscillatore " + juce::String(oscNum),
                juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                "*.wav;*.wave"
            );

            fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, oscNum] (const juce::FileChooser& chooser)
                {
                    auto file = chooser.getResult();
                    if (file.existsAsFile())
                    {
                        audioProcessor.loadAudioFile (file, oscNum - 1);
                    }
                });
        };
    }; // <-- Questo punto e virgola chiude la lambda setupButton

    setupButton (*loadButtonOsc1, 1);
    setupButton (*loadButtonOsc2, 2);
    setupButton (*loadButtonOsc3, 3);

    resized();
} // <-- Questa chiude DEFINITIVAMENTE il Costruttore SambucaAudioProcessorEditor

SambucaAudioProcessorEditor::~SambucaAudioProcessorEditor()
{
    for (auto& cs : connectedSliders)
    {
        if (cs != nullptr && cs->slider != nullptr)
            cs->slider->setLookAndFeel (nullptr);
    }
} // <-- Questa chiude il Distruttore

void SambucaAudioProcessorEditor::createAndConnectKnob (const juce::String& parameterID, const juce::String& sectionName, const juce::String& displayName)
{
    auto cs = std::make_unique<ConnectedSlider>();
    cs->slider = std::make_unique<juce::Slider>();
    cs->label = std::make_unique<juce::Label>();
    cs->section = sectionName;
    cs->cleanName = displayName;

    // Configura lo Slider
    if (sectionName == "GLOBAL_SLIDER") {
        cs->slider->setSliderStyle (juce::Slider::LinearHorizontal); // Morphing lineare!
    } else {
        cs->slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    }
    cs->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 14);
    addAndMakeVisible (*cs->slider);

    // Configura la Label (Scritta sopra il knob)
    cs->label->setText (displayName, juce::dontSendNotification);
    cs->label->setFont (juce::Font (11.0f, juce::Font::bold));
    cs->label->setJustificationType (juce::Justification::centred);
    cs->label->setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (*cs->label);

    cs->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, parameterID, *cs->slider);

    connectedSliders.push_back (std::move(cs)); 
}

void SambucaAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF23272A)); 

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    
    // Testi Macro-aree riposizionati strategicamente fuori dal riquadro del logo
    g.drawText ("OSCILLATORS CONTROLS", 340, 25, 300, 20, juce::Justification::left);
    g.drawText ("FILTERS", 30, 240, 200, 20, juce::Justification::left);
    g.drawText ("LFOs", 30, 420, 200, 20, juce::Justification::left);
    g.drawText ("EFFECTS & MASTER", 680, 25, 300, 20, juce::Justification::left);

    // Disegniamo temporaneamente un rettangolo dove andrà l'oscilloscopio/quadrante custom
    g.setColour (juce::Colours::dimgrey);
    g.drawRect (30, 30, 260, 150, 1);
    g.setFont (12.0f);
    g.drawText ("[ SPAZIO LOGO / VISUALIZZATORE ]", 30, 90, 260, 20, juce::Justification::centred);
}

void SambucaAudioProcessorEditor::resized()
{
    // Limiti e bordi generali chiesti dall'utente
    const int margin = 30;
    const int knobSize = 55;
    const int labelHeight = 15;
    const int totalControlHeight = knobSize + labelHeight;

    int oscCount = 0;
    int filterCount = 0;
    int lfoCount = 0;
    int fxCount = 0;
    int globalCount = 0;

    // Posizionamento Pulsanti Load WAV sotto lo spazio logo
    int btnW = 80;
    int btnH = 22;
    loadButtonOsc1->setBounds (30, 195, btnW, btnH);
    loadButtonOsc2->setBounds (120, 195, btnW, btnH);
    loadButtonOsc3->setBounds (210, 195, btnW, btnH);

    for (const auto& cs : connectedSliders)
    {
        if (cs->slider == nullptr) continue;

        // Griglia OSCILLATORI (Parte centrale dello schermo)
        if (cs->section == "OSC")
        {
            int row = oscCount / 4;
            int col = oscCount % 4;
            int x = 340 + (col * (knobSize + 25));
            int y = 55 + (row * (totalControlHeight + 15));
            
            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            oscCount++;
        }
        // Griglia FILTRI (In basso a sinistra, bilanciata per non uscire dallo schermo)
        else if (cs->section == "FILTER")
        {
            int row = filterCount / 4;
            int col = filterCount % 4;
            int x = margin + (col * (knobSize + 22));
            int y = 270 + (row * (totalControlHeight + 15));

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            filterCount++;
        }
        // Griglia LFO (In fondo a sinistra, sopra il margine di sicurezza)
        else if (cs->section == "LFO")
        {
            int row = lfoCount / 3;
            int col = lfoCount % 3;
            int x = margin + (col * (knobSize + 22));
            int y = 450 + (row * (totalControlHeight + 10));

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            lfoCount++;
        }
        // Griglia FX (In alto a destra)
        else if (cs->section == "FX")
        {
            int row = fxCount / 2;
            int col = fxCount % 2;
            int x = 680 + (col * (knobSize + 40));
            int y = 55 + (row * (totalControlHeight + 20));

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            fxCount++;
        }
        // Controlli Generali (Sotto la sezione FX)
        else if (cs->section == "GLOBAL")
        {
            int x = 680 + (globalCount * (knobSize + 40));
            int y = 240;

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            globalCount++;
        }
        // LO SLIDER DI MORPHING LINEARE! Posizionato lungo e comodo sotto gli oscillatori
        else if (cs->section == "GLOBAL_SLIDER")
        {
            cs->label->setBounds (340, 240, 300, labelHeight);
            cs->slider->setBounds (340, 240 + labelHeight, 295, 25);
        }
    }
}
