#include "PluginProcessor.h"
#include "PluginEditor.h"

SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. ALLOCAZIONE DEI PULSANTI (Deve essere in cima per evitare crash in resized())
    loadButtonOsc1 = std::make_unique<juce::TextButton> ("LOAD WAV 1");
    loadButtonOsc2 = std::make_unique<juce::TextButton> ("LOAD WAV 2");
    loadButtonOsc3 = std::make_unique<juce::TextButton> ("LOAD WAV 3");
    
    addAndMakeVisible (*loadButtonOsc1);
    addAndMakeVisible (*loadButtonOsc2);
    addAndMakeVisible (*loadButtonOsc3);

    // Impostiamo la dimensione fissa ideale per la griglia spaziosa
    setSize (1200, 700);

    // 2. CREAZIONE E CONNESSIONE DEI CONTROLLI
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

    // Globali & ADSR
    createAndConnectKnob ("wavetableMorph", "GLOBAL_SLIDER", "Wave Morph"); 
    createAndConnectKnob ("masterVolume", "GLOBAL", "Master Vol");
    createAndConnectKnob ("envTimeScale", "GLOBAL", "Env Scale");

    createAndConnectKnob ("attack", "ADSR", "Attack");
    createAndConnectKnob ("decay", "ADSR", "Decay");
    createAndConnectKnob ("sustain", "ADSR", "Sustain");
    createAndConnectKnob ("release", "ADSR", "Release");

    // Chiamata di sicurezza al ridimensionamento nativo
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
    if (audioProcessor.apvts.getParameter (parameterID) == nullptr) return;

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
    
    cs->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 14);
    addAndMakeVisible (*cs->slider);

    cs->label->setText (displayName, juce::dontSendNotification);
    cs->label->setFont (juce::FontOptions (12.0f, juce::Font::bold));
    cs->label->setJustificationType (juce::Justification::centred);
    cs->label->setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (*cs->label);

    cs->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, parameterID, *cs->slider);

    connectedSliders.push_back (std::move(cs)); 
}

void SambucaAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Sfondo principale scuro
    g.fillAll (juce::Colour (0xFF16151F)); 

    // Riquadri delle macro-sezioni per dare l'effetto rack hardware orditanto
    g.setColour (juce::Colour (0xFF22202F));
    
    // Area Oscillatori (Intera striscia superiore)
    g.fillRect (15, 15, 1170, 200);
    // Area Centrale (Filtri e LFO affiancati)
    g.fillRect (15, 235, 1170, 210);
    // Area Inferiore (Effetti, ADSR e Master)
    g.fillRect (15, 465, 1170, 210);

    // Disegniamo i contorni e le scritte identificative delle aree
    g.setColour (juce::Colour (0xFF35324A));
    g.drawRect (15, 15, 1170, 200, 1);
    g.drawRect (15, 235, 1170, 210, 1);
    g.drawRect (15, 465, 1170, 210, 1);

    // Titoli delle Sezioni principali
    g.setColour (juce::Colours::orange);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("SAMBUCA SYNTH ENGINE - OSCILLATORS", 30, 22, 300, 20, juce::Justification::left);
    g.drawText ("FILTERS & MODULATIONS", 30, 242, 300, 20, juce::Justification::left);
    g.drawText ("EFFECTS, ENVELOPE & OUTPUT MASTER", 30, 472, 300, 20, juce::Justification::left);
}

void SambucaAudioProcessorEditor::resized()
{
    auto totalArea = getLocalBounds().reduced(15);

    // Ricreiamo esattamente le tre partizioni per mappare i moduli interni
    juce::Rectangle<int> areaOsc    = totalArea.removeFromTop(200);
    totalArea.removeFromTop(20);
    juce::Rectangle<int> areaMiddle = totalArea.removeFromTop(210);
    totalArea.removeFromTop(20);
    juce::Rectangle<int> areaBottom = totalArea;

    // --- POSIZIONAMENTO PULSANTI LOAD (In riga orizzontale pulita a sinistra) ---
    int btnW = 90; int btnH = 24;
    int btnY = areaOsc.getY() + 50;
    if (loadButtonOsc1 != nullptr) loadButtonOsc1->setBounds (30, btnY, btnW, btnH);
    if (loadButtonOsc2 != nullptr) loadButtonOsc2->setBounds (130, btnY, btnW, btnH);
    if (loadButtonOsc3 != nullptr) loadButtonOsc3->setBounds (230, btnY, btnW, btnH);

    // Variabili di incremento per gestire le colonne e distanziare i parametri
    int oscIdx = 0, filterIdx = 0, lfoIdx = 0, fxIdx = 0, adsrIdx = 0, globalIdx = 0;

    // Dimensioni generose dei singoli "moduli" knob per non far accavallare i testi
    int kw = 85; 
    int kh = 110; 

    for (const auto& cs : connectedSliders)
    {
        if (cs->slider == nullptr) continue;

        juce::Rectangle<int> targetBounds;

        if (cs->section == "OSC")
        {
            // Ogni oscillatore (composto da 4 knob) si prende una riga orizzontale isolata. 
            // Invece di fare una griglia cumulativa, li dividiamo matematicamente per indice d'appartenenza.
            int oscNumber = oscIdx / 4; // Determina se fa parte di OSC 1, 2 o 3
            int paramIdx  = oscIdx % 4; // Parametro interno (Wave, Vol, Pitch, Loop)
            
            // X parte da 360 per superare i bottoni di caricamento.
            // Ogni blocco oscillatore ha uno stacco orizzontale di 275 pixel l'uno dall'altro
            int xPos = areaOsc.getX() + 340 + (oscNumber * 275) + (paramIdx * 65);
            int yPos = areaOsc.getY() + 45;
            
            targetBounds = juce::Rectangle<int> (xPos, yPos, 62, kh);
            oscIdx++;
        }
        else if (cs->section == "FILTER")
        {
            // Filtri separati visivamente in due colonne distinte orizzontali
            int filterNumber = filterIdx / 4; // Filtro 1 o Filtro 2
            int paramIdx     = filterIdx % 4;
            int xPos = areaMiddle.getX() + 20 + (filterNumber * 260) + (paramIdx * 62);
            
            targetBounds = juce::Rectangle<int> (xPos, areaMiddle.getY() + 50, 58, kh);
            filterIdx++;
        }
        else if (cs->section == "LFO")
        {
            // LFO posizionati nella metà destra dell'interfaccia centrale (X a partire da 580)
            int lfoNumber = lfoIdx / 3; // LFO 1, 2 o 3
            int paramIdx  = lfoIdx % 3;
            int xPos = areaMiddle.getX() + 580 + (lfoNumber * 200) + (paramIdx * 62);
            
            targetBounds = juce::Rectangle<int> (xPos, areaMiddle.getY() + 50, 58, kh);
            lfoIdx++;
        }
        else if (cs->section == "FX")
        {
            // Riga inferiore, blocco a sinistra
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 20 + (fxIdx * 90), areaBottom.getY() + 50, kw, kh);
            fxIdx++;
        }
        else if (cs->section == "ADSR")
        {
            // Riga inferiore, blocco centrale nettamente separato dagli FX
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 450 + (adsrIdx * 90), areaBottom.getY() + 50, kw, kh);
            adsrIdx++;
        }
        else if (cs->section == "GLOBAL")
        {
            // Riga inferiore, blocco finale a destra dedicato al Master e alla scala
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 900 + (globalIdx * 95), areaBottom.getY() + 50, kw, kh);
            globalIdx++;
        }
        else if (cs->section == "GLOBAL_SLIDER") 
        {
            // Lo slider orizzontale del morphing è posizionato sotto i pulsanti load (X:30, Y:+100)
            targetBounds = juce::Rectangle<int> (areaOsc.getX() + 15, areaOsc.getY() + 105, 290, 50);
        }

        // Assegnamento definitivo geometrico: le label hanno più respiro e stanno nettamente sopra
        if (!targetBounds.isEmpty())
        {
            if (cs->section == "GLOBAL_SLIDER")
            {
                cs->label->setBounds (targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), 18);
                cs->slider->setBounds (targetBounds.getX(), targetBounds.getY() + 18, targetBounds.getWidth(), 25);
            }
            else
            {
                // Larghezza allargata di 20px per le label rispetto ai knob, centrando il testo ed evitando sovrapposizioni laterali
                cs->label->setBounds (targetBounds.getX() - 10, targetBounds.getY(), targetBounds.getWidth() + 20, 18);
                cs->slider->setBounds (targetBounds.getX(), targetBounds.getY() + 22, targetBounds.getWidth(), targetBounds.getHeight() - 22);
            }
        }
    }
}
