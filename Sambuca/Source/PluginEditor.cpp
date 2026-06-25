#include "PluginProcessor.h"
#include "PluginEditor.h"

SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. IMPOSTA LE DIMENSIONI DELLA FINESTRA (Fondamentale, altrimenti resta 0x0)
    setSize (1024, 600);

    // Rimossa la logica di caricamento file corrotta. Usiamo lo sfondo solido nel paint.
    backgroundImage = juce::Image(); 

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
    // Scollega in sicurezza il LookAndFeel standard
    for (auto& cs : connectedSliders)
    {
        if (cs.slider != nullptr)
            cs.slider->setLookAndFeel(nullptr);
    }
}

void SambucaAudioProcessorEditor::createAndConnectKnob (const juce::String& parameterID, const juce::String& sectionName)
{
    ConnectedSlider cs;
    cs.slider = std::make_unique<juce::Slider>();
    cs.section = sectionName;

    cs.slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    
    // Ripristiniamo la TextBox standard in basso così vedi i valori numerici dei parametri
    cs.slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    
    // Escludiamo momentaneamente sambucaLookAndFeel per usare la grafica nativa JUCE
    // cs.slider->setLookAndFeel(&sambucaLookAndFeel); 

    cs.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, parameterID, *cs.slider);

    addAndMakeVisible(*cs.slider);
    connectedSliders.push_back(std::move(cs)); 
}

void SambucaAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Sfondo solido scuro di fallback per rendere visibili i controlli standard
    g.fillAll (juce::Colour (0xFF23272A)); 

    // Disegna etichette di testo per identificare le zone macro del synth
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawText ("OSCILLATORS", 300, 5, 200, 20, juce::Justification::left);
    g.drawText ("FILTERS", 8, 200, 200, 20, juce::Justification::left);
    g.drawText ("LFOs", 8, 360, 200, 20, juce::Justification::left);
    g.drawText ("FX", 680, 5, 200, 20, juce::Justification::left);
}

void SambucaAudioProcessorEditor::resized()
{
    float border = 25.0f; // Alzato leggermente per dare spazio ai testi dei parametri
    
    int oscStartX = 300; 
    int filterStartX = 10;
    
    int rowHeight = 85;   // Aumentato per contenere la text box del valore dello slider
    int knobSize = 65;

    int oscCount = 0;
    int filterCount = 0;
    int lfoCount = 0;
    int fxCount = 0;
    int globalCount = 0;

    for (auto& cs : connectedSliders)
    {
        if (cs.section == "OSC")
        {
            int row = oscCount / 4;
            int col = oscCount % 4;
            cs.slider->setBounds(oscStartX + (col * (knobSize + 20)), border + (row * rowHeight), knobSize, knobSize + 15);
            oscCount++;
        }
        else if (cs.section == "FILTER")
        {
            int row = filterCount / 4;
            int col = filterCount % 4;
            cs.slider->setBounds(filterStartX + (col * (knobSize + 20)), 230 + (row * rowHeight), knobSize, knobSize + 15);
            filterCount++;
        }
        else if (cs.section == "LFO")
        {
            int row = lfoCount / 3;
            int col = lfoCount % 3;
            cs.slider->setBounds(filterStartX + (col * (knobSize + 20)), 390 + (row * rowHeight), knobSize, knobSize + 15);
            lfoCount++;
        }
        else if (cs.section == "FX")
        {
            cs.slider->setBounds(680 + (fxCount * (knobSize + 20)), border, knobSize, knobSize + 15);
            fxCount++;
        }
        else if (cs.section == "GLOBAL")
        {
            cs.slider->setBounds(680 + (globalCount * (knobSize + 20)), 140, knobSize, knobSize + 15);
            globalCount++;
        }
    }
}
