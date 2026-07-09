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

    // LFOs (LFO 1, 2, 3) Confinati nella nuova colonna
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

    // Inviluppo ADSR
    createAndConnectKnob ("attack", "ADSR", "Attack");
    createAndConnectKnob ("decay", "ADSR", "Decay");
    createAndConnectKnob ("sustain", "ADSR", "Sustain");
    createAndConnectKnob ("release", "ADSR", "Release");

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
                *.wav;*.wave
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

void SambucaAudioProcessorEditor::createAndConnectKnob (const juce::String& parameterID, const juce::String& sectionName, const juce::String& displayName)
{
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

    if (sectionName == "GLOBAL_SLIDER") {
        cs->slider->setSliderStyle (juce::Slider::LinearHorizontal); 
    } else {
        cs->slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    }
    cs->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 14);
    addAndMakeVisible (*cs->slider);

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
    
    // Titoli riallineati alle macro aree stabili
    g.drawText ("OSCILLATORS CONTROLS", 340, 25, 300, 20, juce::Justification::left);
    g.drawText ("FILTERS", 30, 240, 200, 20, juce::Justification::left);
    g.drawText ("LFO COLUMN", 849, 25, 150, 20, juce::Justification::centred);
    g.drawText ("EFFECTS & MASTER (SPOSTATI -100px)", 700, 125, 300, 20, juce::Justification::left);
    g.drawText ("PAD X/Y", 656, 355, 343, 20, juce::Justification::left);
    g.drawText ("ENVELOPE (ADSR)", 656, 420, 342, 20, juce::Justification::left);
    
    g.setColour (juce::Colours::dimgrey);
    g.drawRect (30, 30, 260, 150, 1); // Logo/Sample Box
    
    // Spazio Visualizzazione Onde / Display Centrale
    g.drawRect (340, 380, 290, 180, 1);
    g.setFont (12.0f);
    g.drawText ("[ SPAZIO VISUALIZZAZIONE / ONDE ]", 340, 460, 290, 20, juce::Justification::centred);

    // Cornici di debug per la geometria d'angolo fissa richiesti
    g.drawRect (656, 380, 343, 59, 1);  // Box PAD X/Y
    g.drawRect (656, 442, 342, 128, 1); // Box ADSR
}

void SambucaAudioProcessorEditor::resized()
{
    const int knobSize = 55;
    const int labelHeight = 15;
    const int totalControlHeight = knobSize + labelHeight;

    int oscCount = 0;
    int filterCount = 0;
    int lfoCount = 0;
    int fxCount = 0;
    int globalCount = 0;
    int adsrCount = 0;

    // Pulsanti Load WAV
    int btnW = 80;
    int btnH = 22;
    if (loadButtonOsc1 != nullptr) loadButtonOsc1->setBounds (30, 195, btnW, btnH);
    if (loadButtonOsc2 != nullptr) loadButtonOsc2->setBounds (120, 195, btnW, btnH);
    if (loadButtonOsc3 != nullptr) loadButtonOsc3->setBounds (210, 195, btnW, btnH);

    for (const auto& cs : connectedSliders)
    {
        if (cs->slider == nullptr) continue;

        // 1. OSCILLATORI (Invariati al centro-alto)
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
        // 2. FILTRI (Invariati a sinistra)
        else if (cs->section == "FILTER")
        {
            int row = filterCount / 2;
            int col = filterCount % 2;
            int x = 25 + (col * (knobSize + 25));
            int y = 270 + (row * (totalControlHeight + 15));

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            filterCount++;
        }
        // 3. NUOVA COLONNA LFO (In alto a destra confinata: w:150px h:329px)
        else if (cs->section == "LFO")
        {
            int row = lfoCount / 2; // Layout compatto a 2 colonne interne alla sezione
            int col = lfoCount % 2;
            int x = 849 + (col * (knobSize + 10));
            int y = 55 + (row * (totalControlHeight + 12));

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            lfoCount++;
        }
        // 4. SEZIONE FX / MODULATORE (Abbassata di 100px per evitare collisioni)
        else if (cs->section == "FX")
        {
            int row = fxCount / 2;
            int col = fxCount % 2;
            int x = 656 + (col * (knobSize + 25));
            int y = 155 + (row * (totalControlHeight + 15)); // Incrementato di 100px da 55 originale

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            fxCount++;
        }
        // 5. ANGOLO IN BASSO A DESTRA (ADSR fissa nel riquadro w:342, h:128)
        else if (cs->section == "ADSR")
        {
            int col = adsrCount % 4; 
            int x = 656 + (col * (knobSize + 24)); // Distribuisce i 4 controlli uniformemente in 342px
            int y = 460; // Posizionamento verticale centrato nel box h:128 (Safe Zone rispettata)

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            adsrCount++;
        }
        // 6. CONTROLLI GLOBALI
        else if (cs->section == "GLOBAL")
        {
            int x = 656 + (globalCount * (knobSize + 25));
            int y = 290; // Abbassato coerentemente

            cs->label->setBounds (x, y, knobSize, labelHeight);
            cs->slider->setBounds (x, y + labelHeight, knobSize, knobSize);
            globalCount++;
        }
        // 7. MACRO MORPHING SLIDER
        else if (cs->section == "GLOBAL_SLIDER")
        {
            cs->label->setBounds (340, 220, 300, labelHeight);
            cs->slider->setBounds (340, 220 + labelHeight, 295, 25);
        }
    }
}
