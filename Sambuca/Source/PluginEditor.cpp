#include "PluginProcessor.h"
#include "PluginEditor.h"

SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Ingrandiamo la finestra per evitare qualsiasi accavallamento
    setSize (1200, 700);

    // 1. CREAZIONE E CONNESSIONE MANUALE DEI CONTROLLI
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

    // 2. PULSANTI LOAD WAV
    loadButtonOsc1 = std::make_unique<juce::TextButton> ("LOAD WAV 1");
    loadButtonOsc2 = std::make_unique<juce::TextButton> ("LOAD WAV 2");
    loadButtonOsc3 = std::make_unique<juce::TextButton> ("LOAD WAV 3");
    
    addAndMakeVisible (*loadButtonOsc1);
    addAndMakeVisible (*loadButtonOsc2);
    addAndMakeVisible (*loadButtonOsc3);

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
    g.fillAll (juce::Colour (0xFF1C1A27)); // Sfondo scuro omogeneo uniforme
}

void SambucaAudioProcessorEditor::resized()
{
    // RESET COMPLETO CON JUCE::GRID per eliminare gli accavallamenti
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px (15);
    grid.columnGap = juce::Grid::Px (15);

    // Definiamo 3 grandi righe flessibili e 3 colonne principali
    grid.templateRows = { juce::Grid::TrackInfo (juce::Grid::Fr (1)), 
                          juce::Grid::TrackInfo (juce::Grid::Fr (1)), 
                          juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
                          
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)), 
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)), 
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)) };

    // Creiamo dei contenitori logici temporanei per raggruppare i componenti
    juce::Rectangle<int> areaOsc = getLocalBounds().removeFromTop(220).reduced(10);
    juce::Rectangle<int> areaMiddle = getLocalBounds().removeFromTop(220).reduced(10);
    juce::Rectangle<int> areaBottom = getLocalBounds().reduced(10);

    // --- POSIZIONAMENTO PULSANTI LOAD (In alto a sinistra sopra gli OSC) ---
    int btnW = 90; int btnH = 24;
    loadButtonOsc1->setBounds (areaOsc.getX(), areaOsc.getY(), btnW, btnH);
    loadButtonOsc2->setBounds (areaOsc.getX() + 100, areaOsc.getY(), btnW, btnH);
    loadButtonOsc3->setBounds (areaOsc.getX() + 200, areaOsc.getY(), btnW, btnH);

    // --- GRIGLIA AUTOMATICA PER I MANOPOLINI ---
    int oscIdx = 0, filterIdx = 0, lfoIdx = 0, fxIdx = 0, adsrIdx = 0;

    for (const auto& cs : connectedSliders)
    {
        if (cs->slider == nullptr) continue;

        juce::Rectangle<int> targetBounds;

        if (cs->section == "OSC")
        {
            // Disposti in fila nella metà destra dell'area superiore
            int col = oscIdx % 12;
            targetBounds = juce::Rectangle<int> (areaOsc.getX() + 300 + (col * 70), areaOsc.getY(), 65, 85);
            oscIdx++;
        }
        else if (cs->section == "FILTER")
        {
            // Filtri affiancati in orizzontale (non più in una colonna stretta)
            int col = filterIdx % 8;
            targetBounds = juce::Rectangle<int> (areaMiddle.getX() + (col * 75), areaMiddle.getY() + 10, 70, 85);
            filterIdx++;
        }
        else if (cs->section == "LFO")
        {
            int col = lfoIdx % 9;
            targetBounds = juce::Rectangle<int> (areaMiddle.getX() + 600 + (col * 65), areaMiddle.getY() + 10, 60, 80);
            lfoIdx++;
        }
        else if (cs->section == "FX")
        {
            int col = fxIdx % 4;
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + (col * 75), areaBottom.getY() + 10, 70, 85);
            fxIdx++;
        }
        else if (cs->section == "ADSR")
        {
            int col = adsrIdx % 4;
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 400 + (col * 75), areaBottom.getY() + 10, 70, 85);
            adsrIdx++;
        }
        else if (cs->section == "GLOBAL")
        {
            // Separato dal riverbero e dal master per evitare collisioni
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 800, areaBottom.getY() + 10, 70, 85);
        }
        else if (cs->section == "GLOBAL_SLIDER") // Slide del morphing abbassata
        {
            targetBounds = juce::Rectangle<int> (areaOsc.getX(), areaOsc.getY() + 140, 400, 30);
        }

        if (!targetBounds.isEmpty())
        {
            cs->slider->setBounds (targetBounds.getX(), targetBounds.getY() + 15, targetBounds.getWidth(), targetBounds.getHeight() - 15);
            cs->label->setBounds (targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), 15);
        }
    }

    // Nota: I componenti dell'oscilloscopio e del Pad X/Y non sono stati posizionati qui 
    // perché non sono istanziati in questa classe.
}
