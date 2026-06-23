#include "PluginProcessor.h"
#include "PluginEditor.h"

SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. Carica lo sfondo con il tuo logo
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);
    setSize (1024, 600);

    // 2. CREAZIONE E MAPPATURA AUTOMATICA DI TUTTI I 34 PARAMETRI
    
    // Generazione automatica per i 3 Oscillatori
    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "osc" + juce::String(i);
        createAndConnectKnob (prefix + "Waveform", "OSC");
        createAndConnectKnob (prefix + "Volume", "OSC");
        createAndConnectKnob (prefix + "Pitch", "OSC");
        createAndConnectKnob (prefix + "StartLoop", "OSC");
    }

    // Generazione automatica per i 2 Filtri
    for (int i = 1; i <= 2; ++i)
    {
        juce::String prefix = "filter" + juce::String(i);
        createAndConnectKnob (prefix + "Type", "FILTER");
        createAndConnectKnob (prefix + "Cutoff", "FILTER");
        createAndConnectKnob (prefix + "Resonance", "FILTER");
        createAndConnectKnob (prefix + "Drive", "FILTER");
    }

    // Generazione automatica per i 3 LFO
    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "lfo" + juce::String(i);
        createAndConnectKnob (prefix + "Rate", "LFO");
        createAndConnectKnob (prefix + "Waveform", "LFO");
        createAndConnectKnob (prefix + "Amount", "LFO");
    }

    // Effetti
    createAndConnectKnob ("delayTime", "FX");
    createAndConnectKnob ("delayFeedback", "FX");
    createAndConnectKnob ("reverbSize", "FX");
    createAndConnectKnob ("fxMix", "FX");

    // Globali & Inviluppo
    createAndConnectKnob ("wavetableMorph", "GLOBAL");
    createAndConnectKnob ("masterVolume", "GLOBAL");
    createAndConnectKnob ("envTimeScale", "GLOBAL");
}

SambucaAudioProcessorEditor::~SambucaAudioProcessorEditor()
{
    // Scollega in sicurezza il LookAndFeel per evitare crash di memoria
    for (auto& cs : connectedSliders)
    {
        if (cs.slider != nullptr)
            cs.slider->setLookAndFeel(nullptr);
    }
}

// Funzione helper che crea lo slider, lo collega ad APVTS e gli applica il tuo LookAndFeel
void SambucaAudioProcessorEditor::createAndConnectKnob (const juce::String& parameterID, const juce::String& sectionName)
{
    ConnectedSlider cs;
    cs.slider = std::make_unique<juce::Slider>();
    cs.section = sectionName;

    // Imposta lo stile di trascinamento standard per i knob ad alta precisione
    cs.slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    cs.slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    cs.slider->setLookAndFeel(&sambucaLookAndFeel); // Applica la tua estetica (base + glow)

    // JUCE mappa tutto da solo qui legando il controllo all'albero dei parametri
    cs.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, parameterID, *cs.slider);

    addAndMakeVisible(*cs.slider);
    connectedSliders.push_back(std::move(cs)); // Conserva nella lista
}

void SambucaAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (backgroundImage.isValid())
    {
        g.drawImageWithin(backgroundImage, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::fillDestination);
    }
    else
    {
        g.fillAll (juce::Colours::darkgrey);
    }
}

void SambucaAudioProcessorEditor::resized()
{
    float border = 8.5f;
    
    // Coordinate per organizzare la scacchiera dei controlli
    int oscStartX = 300; // Inizia dopo l'area logo (279 + 8.5)
    int filterStartX = border;
    int lfoStartX = border;
    
    int rowHeight = 70;
    int knobSize = 60;

    int oscCount = 0;
    int filterCount = 0;
    int lfoCount = 0;
    int fxCount = 0;
    int globalCount = 0;

    // Distribuzione automatica di tutti i 34 knob in base alla loro sezione
    for (auto& cs : connectedSliders)
    {
        if (cs.section == "OSC")
        {
            // Dispone i 3 oscillatori su 3 righe verticali separate, partendo da X = 300
            int row = oscCount / 4;
            int col = oscCount % 4;
            cs.slider->setBounds(oscStartX + (col * (knobSize + 15)), border + (row * rowHeight), knobSize, knobSize);
            oscCount++;
        }
        else if (cs.section == "FILTER")
        {
            // Filtri posizionati sotto l'area del logo (Y > 150)
            int row = filterCount / 4;
            int col = filterCount % 4;
            cs.slider->setBounds(filterStartX + (col * (knobSize + 15)), 220 + (row * rowHeight), knobSize, knobSize);
            filterCount++;
        }
        else if (cs.section == "LFO")
        {
            // LFO disposti in una griglia ordinata accanto ai filtri
            int row = lfoCount / 3;
            int col = lfoCount % 3;
            cs.slider->setBounds(filterStartX + (col * (knobSize + 15)), 380 + (row * rowHeight), knobSize, knobSize);
            lfoCount++;
        }
        else if (cs.section == "FX")
        {
            // Sezione effetti sulla fascia destra del plugin
            cs.slider->setBounds(680 + (fxCount * (knobSize + 15)), border, knobSize, knobSize);
            fxCount++;
        }
        else if (cs.section == "GLOBAL")
        {
            // Controlli master e scala temporale in basso a destra
            cs.slider->setBounds(680 + (globalCount * (knobSize + 15)), 120, knobSize, knobSize);
            globalCount++;
        }
    }

    // NOTA: Rimane un intero blocco vuoto gigante al centro (da X: 300 a X: 1024, Y > 220)
    // pronto per ospitare lo schermo a Breakpoint di Absynth che implementeremo!
}