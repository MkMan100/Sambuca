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
    createAndConnectKnob ("wavetableMorph", "GLOBAL_SLIDER", "Wave Morph"); 
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
    }; 

    setupButton (*loadButtonOsc1, 1);
    setupButton (*loadButtonOsc2, 2);
    setupButton (*loadButtonOsc3, 3);

    resized();
} 

SambucaAudioProcessorEditor::~SambucaAudioProcessorEditor()
{
    for (auto& cs : connectedSliders)
    {
        if (cs != nullptr && cs->slider != nullptr)
            cs->slider->setLookAndFeel (nullptr);
    }
} 

// Implementazione corretta allineata alla firma dell'header
void SambucaAudioProcessorEditor::createAndConnectKnob (const juce::String& parameterID, const juce::String& sectionName, const juce::String& displayName)
{
    // CONTROLLO DI SICUREZZA
    if (audioProcessor.apvts.getParameter (parameterID) == nullptr)
    {
        juce::File logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("sambuca_debug.txt");
        logFile.appendText("PARAMETRO MANCANTE: " + parameterID + "\n");
        return; 
    }

    auto cs = std::make_unique<ConnectedSlider>();
    cs->slider = std::make_unique<juce::Slider>();
    cs->label = std::make_unique<juce::Label>();
    cs->section = sectionName;
    cs->cleanName = displayName;

    // Configura lo Slider
    if (sectionName == "GLOBAL_SLIDER") {
        cs->slider->setSliderStyle (juce::Slider::LinearHorizontal); 
    } else {
        cs->slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    }
    cs->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 14);
    addAndMakeVisible (*cs->slider);

    // Configura la Label
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
    
    g.drawText ("OSCILLATORS CONTROLS", 340, 25, 300, 20, juce::Justification::left);
    g.drawText ("FILTERS", 30, 240, 200, 20, juce::Justification::left);
    g.drawText ("LFOs", 700, 350, 200, 20, juce::Justification::left);
    g.drawText ("EFFECTS & MASTER", 700, 25, 300, 20, juce::Justification::left);

    // Spazio vuoto in alto a sinistra (Problema 7)
    g.setColour (juce::Colours::dimgrey);
    g.drawRect (30, 30, 260, 150, 1);
    
    // Spazio Visualizzazione spostato in basso al centro (Problema 7)
    g.drawRect (340, 380, 310, 180, 1);
    g.setFont (12.0f);
    g.drawText ("[ SPAZIO VISUALIZZAZIONE / ONDE ]", 340, 460, 310, 20, juce::Justification::centred);
}

void SambucaAudioProcessorEditor::resized()
{
    const int margin = 25;
    const int knobSize = 55;
    const int labelHeight = 15;
    const int totalControlHeight = knobSize + labelHeight;

    int oscCount = 0;
    int filterCount = 0;
    int lfoCount = 0;
    int fxCount = 0;
    int globalCount = 0;

    // Pulsanti Load WAV sotto lo spazio logo (Invariati)
    int btnW = 80;
    int btnH = 22;
    if (loadButtonOsc1 != nullptr) loadButtonOsc1->setBounds (30, 195, btnW, btnH);
    if (loadButtonOsc2 != nullptr) loadButtonOsc2->setBounds (120, 195, btnW, btnH);
    if (loadButtonOsc3 != nullptr) loadButtonOsc3->setBounds (210, 195, btnW, btnH);

    for (const auto& cs : connectedSliders)
    {
        if (cs->slider == nullptr) continue;

        // Sezione OSCILLATORI (Centro Alto)
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
        // Sezione FILTRI (In basso a sinistra)
        else if (cs->section == "FILTER")
        {
            int row = filterCount / 2;
            int col = filterCount % 2;
            int x = margin + (col * (knobSize + 25));
            int y = 270 + (row * (totalControlHeight + 15));

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            filterCount++;
        }
        // Sezione LFO (Spostata nella COLONNA DESTRA libera - Problema 8 risolto)
        else if (cs->section == "LFO")
        {
            int row = lfoCount / 3;
            int col = lfoCount % 3;
            int x = 700 + (col * (knobSize + 15)); // Spostati a destra lontano dai bordi critici
            int y = 380 + (row * (totalControlHeight + 15));

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            lfoCount++;
        }
        // Sezione FX (In alto a destra)
        else if (cs->section == "FX")
        {
            int row = fxCount / 2;
            int col = fxCount % 2;
            int x = 700 + (col * (knobSize + 25));
            int y = 55 + (row * (totalControlHeight + 15));

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            fxCount++;
        }
        // Controlli Globali
        else if (cs->section == "GLOBAL")
        {
            int x = 700 + (globalCount * (knobSize + 25));
            int y = 240;

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            globalCount++;
        }
        // SLIDER DI MORPHING (Abbassato per evitare accavallamenti - Problema 1 risolto)
        else if (cs->section == "GLOBAL_SLIDER")
        {
            cs->label->setBounds (340, 195, 300, labelHeight);
            cs->slider->setBounds (340, 195 + labelHeight, 295, 25);
        }
    }
}
