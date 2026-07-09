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

    // Ora impostiamo la dimensione
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
    
    cs->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
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
    g.fillAll (juce::Colour (0xFF1C1A27)); 
}

void SambucaAudioProcessorEditor::resized()
{
    // Prendiamo l'intera area della finestra (1200x700)
    auto totalArea = getLocalBounds().reduced(20); // 20px di margine tutto intorno

    // Tagliamo la finestra in 3 grandi fette verticali distinte
    // Ciascuna si prende una fetta pulita senza toccare le altre
    juce::Rectangle<int> areaOsc    = totalArea.removeFromTop(200);
    totalArea.removeFromTop(20); // Spazio vuoto di sicurezza (Gap)
    
    juce::Rectangle<int> areaMiddle = totalArea.removeFromTop(200);
    totalArea.removeFromTop(20); // Spazio vuoto di sicurezza (Gap)
    
    juce::Rectangle<int> areaBottom = totalArea; // Il resto va in basso

    // --- POSIZIONAMENTO PULSANTI LOAD ---
    int btnW = 100; int btnH = 26;
    if (loadButtonOsc1 != nullptr) loadButtonOsc1->setBounds (areaOsc.getX(), areaOsc.getY(), btnW, btnH);
    if (loadButtonOsc2 != nullptr) loadButtonOsc2->setBounds (areaOsc.getX() + 110, areaOsc.getY(), btnW, btnH);
    if (loadButtonOsc3 != nullptr) loadButtonOsc3->setBounds (areaOsc.getX() + 220, areaOsc.getY(), btnW, btnH);

    // Indici per il calcolo delle sotto-griglie
    int oscIdx = 0, filterIdx = 0, lfoIdx = 0, fxIdx = 0, adsrIdx = 0, globalIdx = 0;

    for (const auto& cs : connectedSliders)
    {
        if (cs->slider == nullptr) continue;

        juce::Rectangle<int> targetBounds;

        if (cs->section == "OSC")
        {
            // Spostato a destra di 360px rispetto ai pulsanti Load
            int col = oscIdx % 4;
            int row = oscIdx / 4;
            targetBounds = juce::Rectangle<int> (areaOsc.getX() + 360 + (col * 85), areaOsc.getY() + (row * 85), 75, 80);
            oscIdx++;
        }
        else if (cs->section == "FILTER")
        {
            // Zona sinistra dell'area centrale (Filtri affiancati ordinati)
            int col = filterIdx % 4;
            int row = filterIdx / 4;
            targetBounds = juce::Rectangle<int> (areaMiddle.getX() + (col * 85), areaMiddle.getY() + (row * 85), 75, 80);
            filterIdx++;
        }
        else if (cs->section == "LFO")
        {
            // Zona destra dell'area centrale (LFO distanziati dai filtri)
            int col = lfoIdx % 3;
            int row = lfoIdx / 3;
            targetBounds = juce::Rectangle<int> (areaMiddle.getX() + 550 + (col * 85), areaMiddle.getY() + (row * 85), 75, 80);
            lfoIdx++;
        }
        else if (cs->section == "FX")
        {
            // Riga inferiore, blocco a sinistra
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + (fxIdx * 85), areaBottom.getY() + 10, 75, 80);
            fxIdx++;
        }
        else if (cs->section == "ADSR")
        {
            // Riga inferiore, blocco centrale
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 450 + (adsrIdx * 85), areaBottom.getY() + 10, 75, 80);
            adsrIdx++;
        }
        else if (cs->section == "GLOBAL")
        {
            // Riga inferiore, blocco a destra (Master Vol, ecc. isolati dagli FX)
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 850 + (globalIdx * 85), areaBottom.getY() + 10, 75, 80);
            globalIdx++;
        }
        else if (cs->section == "GLOBAL_SLIDER") 
        {
            // Lo slider orizzontale del morphing è isolato sotto i pulsanti load
            targetBounds = juce::Rectangle<int> (areaOsc.getX(), areaOsc.getY() + 50, 320, 50);
        }

        // Applica le coordinate calcolate in modo che la label stia sopra il knob
        if (!targetBounds.isEmpty())
        {
            cs->label->setBounds (targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), 18);
            cs->slider->setBounds (targetBounds.getX(), targetBounds.getY() + 18, targetBounds.getWidth(), targetBounds.getHeight() - 18);
        }
    }
}
