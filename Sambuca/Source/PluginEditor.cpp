#include "PluginProcessor.h"
#include "PluginEditor.h"

// --- IMPLEMENTAZIONE DEI PANNELLI DI CONTROLLO ---

// 1. Pannello Oscillatori
SambucaAudioProcessorEditor::OscTab::OscTab (SambucaAudioProcessor& p, SambucaAudioProcessorEditor& editor)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text) {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        addAndMakeVisible (l);
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
    };

    setupSlider (v1Slider, l1, "Osc 1 Vol");
    v1Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "osc1Volume", v1Slider);
    setupSlider (p1Slider, l2, "Osc 1 Pitch");
    p1Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "osc1Pitch", p1Slider);
    addAndMakeVisible (w1Combo);
    w1Combo.addItemList (juce::StringArray{"Sine", "Saw", "Square", "Wavetable", "Sample", "Noise"}, 1);
    w1Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "osc1Waveform", w1Combo);
    addAndMakeVisible (l3); l3.setText("Osc 1 Wave", juce::dontSendNotification); l3.setJustificationType(juce::Justification::centred);

    setupSlider (v2Slider, l4, "Osc 2 Vol");
    v2Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "osc2Volume", v2Slider);
    setupSlider (p2Slider, l5, "Osc 2 Pitch");
    p2Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "osc2Pitch", p2Slider);
    addAndMakeVisible (w2Combo);
    w2Combo.addItemList (juce::StringArray{"Sine", "Saw", "Square", "Wavetable", "Sample", "Noise"}, 1);
    w2Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "osc2Waveform", w2Combo);
    addAndMakeVisible (l6); l6.setText("Osc 2 Wave", juce::dontSendNotification); l6.setJustificationType(juce::Justification::centred);

    setupSlider (v3Slider, l7, "Osc 3 Vol");
    v3Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "osc3Volume", v3Slider);
    setupSlider (p3Slider, l8, "Osc 3 Pitch");
    p3Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "osc3Pitch", p3Slider);
    addAndMakeVisible (w3Combo);
    w3Combo.addItemList (juce::StringArray{"Sine", "Saw", "Square", "Wavetable", "Sample", "Noise"}, 1);
    w3Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "osc3Waveform", w3Combo);
    addAndMakeVisible (l9); l9.setText("Osc 3 Wave", juce::dontSendNotification); l9.setJustificationType(juce::Justification::centred);

    setupSlider (morphSlider, l10, "Morphing");
    morphAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "wavetableMorph", morphSlider);
}

void SambucaAudioProcessorEditor::OscTab::resized()
{
    auto area = getLocalBounds().reduced (10);
    int colWidth = area.getWidth() / 4;

    auto col = [this, colWidth, &area](juce::Label& lblName, juce::ComboBox& cmb, juce::Label& lblVol, juce::Slider& sVol, juce::Label& lblPtch, juce::Slider& sPtch) {
        auto colArea = area.removeFromLeft (colWidth);
        lblName.setBounds (colArea.removeFromTop (18));
        cmb.setBounds (colArea.removeFromTop (24).reduced(5, 0));
        
        auto topHalf = colArea.removeFromTop (colArea.getHeight() / 2);
        lblVol.setBounds (topHalf.removeFromTop (15));
        sVol.setBounds (topHalf);
        
        lblPtch.setBounds (colArea.removeFromTop (15));
        sPtch.setBounds (colArea);
    };

    col (l3, w1Combo, l1, v1Slider, l2, p1Slider);
    col (l6, w2Combo, l4, v2Slider, l5, p2Slider);
    col (l9, w3Combo, l7, v3Slider, l8, p3Slider);

    // Colonna Morphing
    auto colMorph = area;
    l10.setBounds (colMorph.removeFromTop (18));
    morphSlider.setBounds (colMorph.reduced(10));
}

// 2. Pannello Filtri
SambucaAudioProcessorEditor::FilterTab::FilterTab (SambucaAudioProcessor& p)
{
    auto setupFilter = [this, &p](int num, juce::Slider& cut, juce::Slider& res, juce::Slider& drv, juce::ComboBox& type,
                                  juce::Label& lCut, juce::Label& lRes, juce::Label& lDrv, juce::Label& lType,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& cAtt,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& rAtt,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& dAtt,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& tAtt) 
    {
        addAndMakeVisible (cut); cut.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); cut.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        addAndMakeVisible (lCut); lCut.setText ("F" + juce::String(num) + " Cutoff", juce::dontSendNotification); lCut.setJustificationType (juce::Justification::centred);
        cAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "filter" + juce::String(num) + "Cutoff", cut);

        addAndMakeVisible (res); res.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); res.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        addAndMakeVisible (lRes); lRes.setText ("F" + juce::String(num) + " Res", juce::dontSendNotification); lRes.setJustificationType (juce::Justification::centred);
        rAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "filter" + juce::String(num) + "Resonance", res);

        addAndMakeVisible (drv); drv.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); drv.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        addAndMakeVisible (lDrv); lDrv.setText ("F" + juce::String(num) + " Drive", juce::dontSendNotification); lDrv.setJustificationType (juce::Justification::centred);
        dAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "filter" + juce::String(num) + "Drive", drv);

        addAndMakeVisible (type); type.addItemList (juce::StringArray{"Lowpass", "Highpass", "Bandpass"}, 1);
        tAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "filter" + juce::String(num) + "Type", type);
        addAndMakeVisible (lType); lType.setText ("Filter " + juce::String(num) + " Type", juce::dontSendNotification); lType.setJustificationType (juce::Justification::centred);
    };

    setupFilter (1, cut1Slider, res1Slider, drive1Slider, type1Combo, l1, l2, l3, l4, cut1Att, res1Att, drive1Att, type1Att);
    setupFilter (2, cut2Slider, res2Slider, drive2Slider, type2Combo, l5, l6, l7, l8, cut2Att, res2Att, drive2Att, type2Att);
}

void SambucaAudioProcessorEditor::FilterTab::resized()
{
    auto area = getLocalBounds().reduced (10);
    int sectionWidth = area.getWidth() / 2;

    auto layoutFilter = [this](juce::Rectangle<int> sArea, juce::Label& lT, juce::ComboBox& cmb, juce::Label& lC, juce::Slider& sC, juce::Label& lR, juce::Slider& sR, juce::Label& lD, juce::Slider& sD) {
        lT.setBounds (sArea.removeFromTop (18));
        cmb.setBounds (sArea.removeFromTop (24).reduced(20, 0));
        
        int itemW = sArea.getWidth() / 3;
        auto c1 = sArea.removeFromLeft (itemW);
        lC.setBounds (c1.removeFromTop (15));
        sC.setBounds (c1);

        auto c2 = sArea.removeFromLeft (itemW);
        lR.setBounds (c2.removeFromTop (15));
        sR.setBounds (c2);

        auto c3 = sArea;
        lD.setBounds (c3.removeFromTop (15));
        sD.setBounds (c3);
    };

    layoutFilter (area.removeFromLeft (sectionWidth).reduced(5), l4, type1Combo, l1, cut1Slider, l2, res1Slider, l3, drive1Slider);
    layoutFilter (area.reduced(5), l8, type2Combo, l5, cut2Slider, l6, res2Slider, l7, drive2Slider);
}

// 3. Pannello ADSR
SambucaAudioProcessorEditor::EnvelopeTab::EnvelopeTab (SambucaAudioProcessor& p)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text) {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        addAndMakeVisible (l);
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
    };

    setupSlider (attSlider, l1, "Attack");
    attAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "attack", attSlider);

    setupSlider (decSlider, l2, "Decay");
    decAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "decay", decSlider);

    setupSlider (susSlider, l3, "Sustain");
    susAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "sustain", susSlider);

    setupSlider (relSlider, l4, "Release");
    relAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "release", relSlider);

    setupSlider (scaleSlider, l5, "Env Scale");
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "envTimeScale", scaleSlider);
}

void SambucaAudioProcessorEditor::EnvelopeTab::resized()
{
    auto area = getLocalBounds().reduced (10);
    int colWidth = area.getWidth() / 5;

    auto place = [colWidth, &area](juce::Label& l, juce::Slider& s) {
        auto col = area.removeFromLeft (colWidth);
        l.setBounds (col.removeFromTop (18));
        s.setBounds (col.reduced (5));
    };

    place (l1, attSlider);
    place (l2, decSlider);
    place (l3, susSlider);
    place (l4, relSlider);
    place (l5, scaleSlider);
}

// 4. Pannello LFO (Tutti e 3 gli LFO sono configurabili qui)
SambucaAudioProcessorEditor::LfoTab::LfoTab (SambucaAudioProcessor& p)
{
    auto setupLfo = [this, &p](int num, juce::Slider& rate, juce::Slider& amt, juce::ComboBox& wave, juce::ComboBox& tgt,
                               juce::Label& lRate, juce::Label& lAmt, juce::Label& lWave, juce::Label& lTgt,
                               std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& rAtt,
                               std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& aAtt,
                               std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& wAtt,
                               std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& tAtt)
    {
        addAndMakeVisible (rate); rate.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); rate.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        rAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "lfo" + juce::String(num) + "Rate", rate);
        addAndMakeVisible (lRate); lRate.setText ("LFO " + juce::String(num) + " Rate", juce::dontSendNotification); lRate.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (amt); amt.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); amt.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        aAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "lfo" + juce::String(num) + "Amount", amt);
        addAndMakeVisible (lAmt); lAmt.setText ("Amount", juce::dontSendNotification); lAmt.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (wave); wave.addItemList (juce::StringArray{"Sine", "Triangle", "Saw", "Random"}, 1);
        wAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "lfo" + juce::String(num) + "Waveform", wave);
        addAndMakeVisible (lWave); lWave.setText ("Shape", juce::dontSendNotification); lWave.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (tgt); tgt.addItemList (juce::StringArray{"None", "Filter Cutoff", "Osc Volume", "Osc Pitch"}, 1);
        tAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "lfo" + juce::String(num) + "Target", tgt);
        addAndMakeVisible (lTgt); lTgt.setText ("Target", juce::dontSendNotification); lTgt.setJustificationType (juce::Justification::centred);
    };

    setupLfo (1, rate1Slider, amt1Slider, wave1Combo, tgt1Combo, l1, l2, l3, l4, rate1Att, amt1Att, wave1Att, tgt1Att);
    setupLfo (2, rate2Slider, amt2Slider, wave2Combo, tgt2Combo, l5, l6, l7, l8, rate2Att, amt2Att, wave2Att, tgt2Att);
    setupLfo (3, rate3Slider, amt3Slider, wave3Combo, tgt3Combo, l9, l10, l11, l12, rate3Att, amt3Att, wave3Att, tgt3Att);
}

void SambucaAudioProcessorEditor::LfoTab::resized()
{
    auto area = getLocalBounds().reduced (10);
    int colWidth = area.getWidth() / 3;

    auto layoutLfo = [colWidth](juce::Rectangle<int> lfoArea, juce::Label& lW, juce::ComboBox& cW, juce::Label& lT, juce::ComboBox& cT, juce::Label& lR, juce::Slider& sR, juce::Label& lA, juce::Slider& sA) {
        auto topRow = lfoArea.removeFromTop (42);
        auto leftTop = topRow.removeFromLeft (colWidth / 2).reduced(3, 0);
        lW.setBounds (leftTop.removeFromTop (15));
        cW.setBounds (leftTop);

        auto rightTop = topRow.reduced(3, 0);
        lT.setBounds (rightTop.removeFromTop (15));
        cT.setBounds (rightTop);

        int sliderW = lfoArea.getWidth() / 2;
        auto c1 = lfoArea.removeFromLeft (sliderW);
        lR.setBounds (c1.removeFromTop (15));
        sR.setBounds (c1);

        auto c2 = lfoArea;
        lA.setBounds (c2.removeFromTop (15));
        sA.setBounds (c2);
    };

    layoutLfo (area.removeFromLeft (colWidth).reduced (5), l3, wave1Combo, l4, tgt1Combo, l1, rate1Slider, l2, amt1Slider);
    layoutLfo (area.removeFromLeft (colWidth).reduced (5), l7, wave2Combo, l8, tgt2Combo, l5, rate2Slider, l6, amt2Slider);
    layoutLfo (area.reduced (5), l11, wave3Combo, l12, tgt3Combo, l9, rate3Slider, l10, amt3Slider);
}

// 5. Pannello FX & Master
SambucaAudioProcessorEditor::FxMasterTab::FxMasterTab (SambucaAudioProcessor& p)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text) {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        addAndMakeVisible (l);
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
    };

    setupSlider (delayTimeSlider, l1, "Delay Time");
    dTimeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "delayTime", delayTimeSlider);

    setupSlider (delayFbSlider, l2, "Delay FB");
    dFbAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "delayFeedback", delayFbSlider);

    setupSlider (reverbSizeSlider, l3, "Reverb Space");
    revAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "reverbSize", reverbSizeSlider);

    setupSlider (fxMixSlider, l4, "FX Mix");
    mixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "fxMix", fxMixSlider);

    setupSlider (masterVolSlider, l5, "Master Volume");
    volAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "masterVolume", masterVolSlider);
}

void SambucaAudioProcessorEditor::FxMasterTab::resized()
{
    auto area = getLocalBounds().reduced (10);
    int colWidth = area.getWidth() / 5;

    auto place = [colWidth, &area](juce::Label& l, juce::Slider& s) {
        auto col = area.removeFromLeft (colWidth);
        l.setBounds (col.removeFromTop (18));
        s.setBounds (col.reduced (5));
    };

    place (l1, delayTimeSlider);
    place (l2, delayFbSlider);
    place (l3, reverbSizeSlider);
    place (l4, fxMixSlider);
    place (l5, masterVolSlider);
}


// --- COSTRUTTORE DELL'EDITOR PRINCIPALE ---

SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), oscilloscope (p)
{
    // Finestra ottimizzata per contenere oscilloscopio + schede
    setSize (700, 480);

    // Oscilloscopio fisso in alto
    addAndMakeVisible (oscilloscope);

    // Inizializza i pannelli
    oscPanel = std::make_unique<OscTab> (audioProcessor, *this);
    filterPanel = std::make_unique<FilterTab> (audioProcessor);
    envelopePanel = std::make_unique<EnvelopeTab> (audioProcessor);
    lfoPanel = std::make_unique<LfoTab> (audioProcessor);
    fxMasterPanel = std::make_unique<FxMasterTab> (audioProcessor);

    // Configura e aggiunge i Tab
    addAndMakeVisible (tabbedComponent);
    tabbedComponent.addTab ("Oscillators", juce::Colours::transparentBlack, oscPanel.get(), false);
    tabbedComponent.addTab ("Filters", juce::Colours::transparentBlack, filterPanel.get(), false);
    tabbedComponent.addTab ("Envelope", juce::Colours::transparentBlack, envelopePanel.get(), false);
    tabbedComponent.addTab ("LFOs", juce::Colours::transparentBlack, lfoPanel.get(), false);
    tabbedComponent.addTab ("FX & Master", juce::Colours::transparentBlack, fxMasterPanel.get(), false);
    
    tabbedComponent.setTabBarPosition (juce::TabbedButtonBar::TabsAtTop);
    tabbedComponent.setCurrentTabIndex (0);
}

SambucaAudioProcessorEditor::~SambucaAudioProcessorEditor() {}

void SambucaAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF1A1926));
}

void SambucaAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // 1. Alloca l'oscilloscopio in alto (120px di altezza)
    auto topArea = area.removeFromTop (120).reduced (10);
    oscilloscope.setBounds (topArea);

    // Divisorio minimo
    area.removeFromTop (5);

    // 2. Tutto il resto dello schermo va al sistema TabbedComponent
    tabbedComponent.setBounds (area.reduced (10, 5));
}
