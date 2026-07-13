#include "PluginProcessor.h"
#include "PluginEditor.h"

SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // --- 1. CONFIGURAZIONE E FUNZIONALITÀ DEI PULSANTI LOAD WAV ---
    auto setupLoadButton = [this](std::unique_ptr<juce::TextButton>& button, int oscIndex)
    {
        button = std::make_unique<juce::TextButton> ("LOAD WAV " + juce::String(oscIndex));
        addAndMakeVisible (*button);
        
        button->onClick = [this, oscIndex]()
        {
            fileChooser = std::make_unique<juce::FileChooser> (
                "Seleziona un file audio per l'Oscillatore " + juce::String(oscIndex),
                juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                "*.wav;*.mp3;*.aif"
            );

            auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
            
            fileChooser->launchAsync (flags, [this, oscIndex] (const juce::FileChooser& chooser)
            {
                auto file = chooser.getResult();
                if (file.existsAsFile())
                {
                    audioProcessor.loadAudioFile (file, oscIndex - 1);
                }
            });
        };
    };

    setupLoadButton (loadButtonOsc1, 1);
    setupLoadButton (loadButtonOsc2, 2);
    setupLoadButton (loadButtonOsc3, 3);

    setSize (1200, 700);

    // --- 2. CREAZIONE E CONNESSIONE DEI CONTROLLI STANDARD ---
    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "osc" + juce::String(i);
        juce::String oscName = "OSC " + juce::String(i) + " ";
        createAndConnectKnob (prefix + "Waveform", "OSC", oscName + "Wave");
        createAndConnectKnob (prefix + "Volume", "OSC", oscName + "Vol");
        createAndConnectKnob (prefix + "Pitch", "OSC", oscName + "Pitch");
        createAndConnectKnob (prefix + "StartLoop", "OSC", oscName + "Loop");
    }

    for (int i = 1; i <= 2; ++i)
    {
        juce::String prefix = "filter" + juce::String(i);
        juce::String fltName = "FLT " + juce::String(i) + " ";
        createAndConnectKnob (prefix + "Type", "FILTER", fltName + "Type");
        createAndConnectKnob (prefix + "Cutoff", "FILTER", fltName + "Cut");
        createAndConnectKnob (prefix + "Resonance", "FILTER", fltName + "Res");
        createAndConnectKnob (prefix + "Drive", "FILTER", fltName + "Drive");
    }

    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "lfo" + juce::String(i);
        juce::String lfoName = "LFO " + juce::String(i) + " ";
        createAndConnectKnob (prefix + "Rate", "LFO", lfoName + "Rate");
        createAndConnectKnob (prefix + "Waveform", "LFO", lfoName + "Wave");
        createAndConnectKnob (prefix + "Amount", "LFO", lfoName + "Amt");
    }

    createAndConnectKnob ("delayTime", "FX", "Delay Time");
    createAndConnectKnob ("delayFeedback", "FX", "Delay Fback");
    createAndConnectKnob ("reverbSize", "FX", "Reverb Size");
    createAndConnectKnob ("fxMix", "FX", "FX Mix");

    createAndConnectKnob ("wavetableMorph", "GLOBAL_SLIDER", "Wave Morph"); 
    createAndConnectKnob ("masterVolume", "GLOBAL", "Master Vol");
    createAndConnectKnob ("envTimeScale", "GLOBAL", "Env Scale");

    createAndConnectKnob ("attack", "ADSR", "Attack");
    createAndConnectKnob ("decay", "ADSR", "Decay");
    createAndConnectKnob ("sustain", "ADSR", "Sustain");
    createAndConnectKnob ("release", "ADSR", "Release");

    // --- 3. CONFIGURAZIONE DEL PAD X/Y BI-DIMENSIONALE ---
    xyPad = std::make_unique<juce::Slider> ();
    xyPad->setSliderStyle(juce::Slider::TwoValueHorizontal); // Imposta uno stile compatibile di appoggio
    xyPad->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    
    // Se dà errore qui, controlla che nel file PluginEditor.h ci sia scritto "SambucaLookAndFeel customLookAndFeel;" sotto private:
    xyPad->setLookAndFeel (&customLookAndFeel); 
    addAndMakeVisible (*xyPad);

    xyLabel = std::make_unique<juce::Label> ("", "X: Morph / Y: Filt 1 Cut");
    xyLabel->setFont (juce::Font (11.0f, juce::Font::bold)); // Corretto FontOptions in Font
    xyLabel->setJustificationType (juce::Justification::centred);
    xyLabel->setColour (juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible (*xyLabel);

    xyXAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "wavetableMorph", *xyPad);
    xyYAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "filter1Cutoff", *xyPad);

    resized();
} 

SambucaAudioProcessorEditor::~SambucaAudioProcessorEditor()
{
    if (xyPad != nullptr)
        xyPad->setLookAndFeel (nullptr);

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
    cs->label->setFont (juce::Font (11.0f, juce::Font::bold)); // Corretto FontOptions in Font
    cs->label->setJustificationType (juce::Justification::centred);
    cs->label->setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (*cs->label);

    cs->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, parameterID, *cs->slider);

    connectedSliders.push_back (std::move(cs)); 
}

void SambucaAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF16151F)); 

    g.setColour (juce::Colour (0xFF22202F));
    g.fillRect (15, 15, 1170, 200);  
    g.fillRect (15, 235, 1170, 210); 
    g.fillRect (15, 465, 1170, 210); 

    g.setColour (juce::Colour (0xFF35324A));
    g.drawRect (15, 15, 1170, 200, 1);
    g.drawRect (15, 235, 1170, 210, 1);
    g.drawRect (15, 465, 1170, 210, 1);

    g.setColour (juce::Colours::orange); // Corretto jures in juce
    g.setFont (juce::Font (13.0f, juce::Font::bold)); // Corretto FontOptions in Font
    g.drawText ("SAMBUCA SYNTH ENGINE - OSCILLATORS", 30, 22, 300, 20, juce::Justification::left);
    g.drawText ("FILTERS & MODULATIONS", 30, 242, 300, 20, juce::Justification::left);
    g.drawText ("EFFECTS, ENVELOPE & OUTPUT MASTER", 30, 472, 300, 20, juce::Justification::left);
}

void SambucaAudioProcessorEditor::resized()
{
    auto totalArea = getLocalBounds().reduced(15);

    juce::Rectangle<int> areaOsc    = totalArea.removeFromTop(200);
    totalArea.removeFromTop(20);
    juce::Rectangle<int> areaMiddle = totalArea.removeFromTop(210);
    totalArea.removeFromTop(20);
    juce::Rectangle<int> areaBottom = totalArea;

    int btnW = 90; int btnH = 24;
    int btnY = areaOsc.getY() + 45;
    if (loadButtonOsc1 != nullptr) loadButtonOsc1->setBounds (30, btnY, btnW, btnH);
    if (loadButtonOsc2 != nullptr) loadButtonOsc2->setBounds (130, btnY, btnW, btnH);
    if (loadButtonOsc3 != nullptr) loadButtonOsc3->setBounds (230, btnY, btnW, btnH);

    int oscIdx = 0, filterIdx = 0, lfoIdx = 0, fxIdx = 0, adsrIdx = 0, globalIdx = 0;
    int kw = 85; 
    int kh = 110; 

    for (const auto& cs : connectedSliders)
    {
        if (cs->slider == nullptr) continue;

        juce::Rectangle<int> targetBounds;

        if (cs->section == "OSC")
        {
            int oscNumber = oscIdx / 4; 
            int paramIdx  = oscIdx % 4; 
            int xPos = areaOsc.getX() + 340 + (oscNumber * 275) + (paramIdx * 65);
            targetBounds = juce::Rectangle<int> (xPos, areaOsc.getY() + 45, 62, kh);
            oscIdx++;
        }
        else if (cs->section == "FILTER")
        {
            int filterNumber = filterIdx / 4; 
            int paramIdx     = filterIdx % 4;
            int xPos = areaMiddle.getX() + 20 + (filterNumber * 240) + (paramIdx * 60);
            targetBounds = juce::Rectangle<int> (xPos, areaMiddle.getY() + 50, 56, kh);
            filterIdx++;
        }
        else if (cs->section == "LFO")
        {
            int lfoNumber = lfoIdx / 3; 
            int paramIdx  = lfoIdx % 3;
            int xPos = areaMiddle.getX() + 720 + (lfoNumber * 150) + (paramIdx * 50);
            targetBounds = juce::Rectangle<int> (xPos, areaMiddle.getY() + 50, 48, kh);
            lfoIdx++;
        }
        else if (cs->section == "FX")
        {
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 20 + (fxIdx * 90), areaBottom.getY() + 50, kw, kh);
            fxIdx++;
        }
        else if (cs->section == "ADSR")
        {
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 450 + (adsrIdx * 90), areaBottom.getY() + 50, kw, kh);
            adsrIdx++;
        }
        else if (cs->section == "GLOBAL")
        {
            targetBounds = juce::Rectangle<int> (areaBottom.getX() + 900 + (globalIdx * 95), areaBottom.getY() + 50, kw, kh);
            globalIdx++;
        }
        else if (cs->section == "GLOBAL_SLIDER") 
        {
            targetBounds = juce::Rectangle<int> (areaOsc.getX() + 15, areaOsc.getY() + 105, 290, 50);
        }

        if (!targetBounds.isEmpty())
        {
            if (cs->section == "GLOBAL_SLIDER")
            {
                cs->label->setBounds (targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), 18);
                cs->slider->setBounds (targetBounds.getX(), targetBounds.getY() + 18, targetBounds.getWidth(), 25);
            }
            else
            {
                cs->label->setBounds (targetBounds.getX() - 10, targetBounds.getY(), targetBounds.getWidth() + 20, 18);
                cs->slider->setBounds (targetBounds.getX(), targetBounds.getY() + 22, targetBounds.getWidth(), targetBounds.getHeight() - 22);
            }
        }
    }

    int padSize = 130;
    int padX = areaMiddle.getX() + 540;
    int padY = areaMiddle.getY() + 55;
    
    if (xyPad != nullptr)   xyPad->setBounds (padX, padY, padSize, padSize);
    if (xyLabel != nullptr) xyLabel->setBounds (padX - 20, padY - 22, padSize + 40, 18);
}
