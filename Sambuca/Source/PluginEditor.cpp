#include "PluginProcessor.h" 
#include "PluginEditor.h"

void SambucaAudioProcessorEditor::setupRotary (juce::Slider& s, juce::Label& l, const juce::String& text) 
    { 
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 18);
        addAndMakeVisible (l); l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred); 
    }

void SambucaAudioProcessorEditor::setupCombo (juce::ComboBox& c, const juce::StringArray& items, juce::Label& l, const juce::String& text) 
    { 
        addAndMakeVisible (c);
        c.addItemList (items, 1);
        addAndMakeVisible (l);
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred); 
    }
SambucaAudioProcessorEditor::SambucaAudioProcessorEditor (SambucaAudioProcessor& p) : AudioProcessorEditor (&p), audioProcessor (p), oscilloscope (p) { setSize (1000, 700); addAndMakeVisible (oscilloscope);

auto makeSection = [this](juce::Label& l, const juce::String& text) {
    addAndMakeVisible (l);
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centredLeft);
    l.setColour (juce::Label::textColourId, juce::Colours::orange);
    l.setFont (juce::Font (14.0f, juce::Font::bold));
};
makeSection (secOscLabel,    "OSC");
makeSection (secFilterLabel, "FILTER");
makeSection (secEnvLabel,    "ADSR");
makeSection (secLfoLabel,    "LFO");
makeSection (secFxLabel,     "FX / MASTER");

// OSCILLATORI
setupRotary (v1Slider, oscL1, "Osc 1 Vol");
v1Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "osc1Volume", v1Slider);
setupRotary (p1Slider, oscL2, "Osc 1 Pitch");
p1Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "osc1Pitch", p1Slider);
setupCombo  (w1Combo, {"Sine","Saw","Square","Wavetable","Sample","Noise"}, oscL3, "Osc 1 Wave");
w1Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "osc1Waveform", w1Combo);

setupRotary (v2Slider, oscL4, "Osc 2 Vol");
v2Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "osc2Volume", v2Slider);
setupRotary (p2Slider, oscL5, "Osc 2 Pitch");
p2Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "osc2Pitch", p2Slider);
setupCombo  (w2Combo, {"Sine","Saw","Square","Wavetable","Sample","Noise"}, oscL6, "Osc 2 Wave");
w2Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "osc2Waveform", w2Combo);

setupRotary (v3Slider, oscL7, "Osc 3 Vol");
v3Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "osc3Volume", v3Slider);
setupRotary (p3Slider, oscL8, "Osc 3 Pitch");
p3Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "osc3Pitch", p3Slider);
setupCombo  (w3Combo, {"Sine","Saw","Square","Wavetable","Sample","Noise"}, oscL9, "Osc 3 Wave");
w3Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "osc3Waveform", w3Combo);

setupRotary (morphSlider, oscL10, "Morphing");
morphAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "wavetableMorph", morphSlider);

// FILTRI
setupRotary (cut1Slider,   fltL1, "F1 Cutoff");
cut1Att   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "filter1Cutoff",    cut1Slider);
setupRotary (res1Slider,   fltL2, "F1 Res");
res1Att   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "filter1Resonance", res1Slider);
setupRotary (drive1Slider, fltL3, "F1 Drive");
drive1Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "filter1Drive",     drive1Slider);
setupCombo  (type1Combo, {"Lowpass","Highpass","Bandpass"}, fltL4, "F1 Type");
type1Att  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "filter1Type", type1Combo);

setupRotary (cut2Slider,   fltL5, "F2 Cutoff");
cut2Att   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "filter2Cutoff",    cut2Slider);
setupRotary (res2Slider,   fltL6, "F2 Res");
res2Att   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "filter2Resonance", res2Slider);
setupRotary (drive2Slider, fltL7, "F2 Drive");
drive2Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "filter2Drive",     drive2Slider);
setupCombo  (type2Combo, {"Lowpass","Highpass","Bandpass"}, fltL8, "F2 Type");
type2Att  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "filter2Type", type2Combo);

// ENVELOPE
setupRotary (attSlider,   envL1, "Attack");
attAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "attack",       attSlider);
setupRotary (decSlider,   envL2, "Decay");
decAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "decay",        decSlider);
setupRotary (susSlider,   envL3, "Sustain");
susAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "sustain",      susSlider);
setupRotary (relSlider,   envL4, "Release");
relAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "release",      relSlider);
setupRotary (scaleSlider, envL5, "Env Scale");
scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "envTimeScale", scaleSlider);

// LFO 1
setupRotary (rate1Slider, lfoL1, "LFO 1 Rate");
rate1Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "lfo1Rate",   rate1Slider);
setupRotary (amt1Slider,  lfoL2, "Amount");
amt1Att  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "lfo1Amount", amt1Slider);
setupCombo  (wave1Combo, {"Sine","Triangle","Saw","Random"}, lfoL3, "Shape");
wave1Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "lfo1Waveform", wave1Combo);
setupCombo  (tgt1Combo, {"None","Filter Cutoff","Osc Volume","Osc Pitch"}, lfoL4, "Target");
tgt1Att  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "lfo1Target", tgt1Combo);

// LFO 2
setupRotary (rate2Slider, lfoL5, "LFO 2 Rate");
rate2Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "lfo2Rate",   rate2Slider);
setupRotary (amt2Slider,  lfoL6, "Amount");
amt2Att  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "lfo2Amount", amt2Slider);
setupCombo  (wave2Combo, {"Sine","Triangle","Saw","Random"}, lfoL7, "Shape");
wave2Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "lfo2Waveform", wave2Combo);
setupCombo  (tgt2Combo, {"None","Filter Cutoff","Osc Volume","Osc Pitch"}, lfoL8, "Target");
tgt2Att  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "lfo2Target", tgt2Combo);

// LFO 3
setupRotary (rate3Slider, lfoL9,  "LFO 3 Rate");
rate3Att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "lfo3Rate",   rate3Slider);
setupRotary (amt3Slider,  lfoL10, "Amount");
amt3Att  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "lfo3Amount", amt3Slider);
setupCombo  (wave3Combo, {"Sine","Triangle","Saw","Random"}, lfoL11, "Shape");
wave3Att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "lfo3Waveform", wave3Combo);
setupCombo  (tgt3Combo, {"None","Filter Cutoff","Osc Volume","Osc Pitch"}, lfoL12, "Target");
tgt3Att  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "lfo3Target", tgt3Combo);

// FX & MASTER
setupRotary (delayTimeSlider,  fxL1, "Delay Time");
dTimeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "delayTime",     delayTimeSlider);
setupRotary (delayFbSlider,    fxL2, "Delay FB");
dFbAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "delayFeedback", delayFbSlider);
setupRotary (reverbSizeSlider, fxL3, "Reverb Space");
revAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "reverbSize",    reverbSizeSlider);
setupRotary (fxMixSlider,      fxL4, "FX Mix");
mixAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "fxMix",         fxMixSlider);
setupRotary (masterVolSlider,  fxL5, "Master Volume");
volAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "masterVolume",  masterVolSlider);
}

SambucaAudioProcessorEditor::~SambucaAudioProcessorEditor() {}

void SambucaAudioProcessorEditor::paint (juce::Graphics& g) 
{ 
    g.fillAll (juce::Colour (0xFF1A1926)); 
    g.setColour (juce::Colour (0xFF35324A)); 
    g.drawRect (logoArea, 1); 
    g.setColour (juce::Colour (0xFF6A6580)); 
    g.setFont (12.0f); 
    g.drawText ("LOGO", logoArea, juce::Justification::centred); 
}

void SambucaAudioProcessorEditor::resized() { auto area = getLocalBounds().reduced (10);

// HEADER: oscilloscopio 250x145 + slot logo 103x100
auto header = area.removeFromTop (150);
oscilloscope.setBounds (header.removeFromLeft (250).withHeight (145));
auto logoSlot = header.removeFromLeft (110);
logoArea = juce::Rectangle<int> (0, 0, 103, 100).withCentre (logoSlot.getCentre());
area.removeFromTop (5);

const int rowH   = area.getHeight() / 5;
const int labelW = 70;

auto placeKnob = [](juce::Rectangle<int> r, juce::Label& l, juce::Slider& s) {
    l.setBounds (r.removeFromTop (16));
    s.setBounds (r.reduced (2));
};
auto placeCombo = [](juce::Rectangle<int> r, juce::Label& l, juce::ComboBox& c) {
    l.setBounds (r.removeFromTop (16));
    r.removeFromTop ((r.getHeight() - 24) / 2);
    c.setBounds (r.removeFromTop (24).reduced (4, 0));
};

// OSC
{
    auto row = area.removeFromTop (rowH);
    secOscLabel.setBounds (row.removeFromLeft (labelW));
    int colW = row.getWidth() / 10;
    placeCombo (row.removeFromLeft (colW).reduced (2), oscL3, w1Combo);
    placeKnob  (row.removeFromLeft (colW).reduced (2), oscL1, v1Slider);
    placeKnob  (row.removeFromLeft (colW).reduced (2), oscL2, p1Slider);
    placeCombo (row.removeFromLeft (colW).reduced (2), oscL6, w2Combo);
    placeKnob  (row.removeFromLeft (colW).reduced (2), oscL4, v2Slider);
    placeKnob  (row.removeFromLeft (colW).reduced (2), oscL5, p2Slider);
    placeCombo (row.removeFromLeft (colW).reduced (2), oscL9, w3Combo);
    placeKnob  (row.removeFromLeft (colW).reduced (2), oscL7, v3Slider);
    placeKnob  (row.removeFromLeft (colW).reduced (2), oscL8, p3Slider);
    placeKnob  (row.reduced (2),                       oscL10, morphSlider);
}

// FILTER
{
    auto row = area.removeFromTop (rowH);
    secFilterLabel.setBounds (row.removeFromLeft (labelW));
    int colW = row.getWidth() / 8;
    placeCombo (row.removeFromLeft (colW).reduced (2), fltL4, type1Combo);
    placeKnob  (row.removeFromLeft (colW).reduced (2), fltL1, cut1Slider);
    placeKnob  (row.removeFromLeft (colW).reduced (2), fltL2, res1Slider);
    placeKnob  (row.removeFromLeft (colW).reduced (2), fltL3, drive1Slider);
    placeCombo (row.removeFromLeft (colW).reduced (2), fltL8, type2Combo);
    placeKnob  (row.removeFromLeft (colW).reduced (2), fltL5, cut2Slider);
    placeKnob  (row.removeFromLeft (colW).reduced (2), fltL6, res2Slider);
    placeKnob  (row.reduced (2),                       fltL7, drive2Slider);
}

// ADSR
{
    auto row = area.removeFromTop (rowH);
    secEnvLabel.setBounds (row.removeFromLeft (labelW));
    int colW = row.getWidth() / 5;
    placeKnob (row.removeFromLeft (colW).reduced (2), envL1, attSlider);
    placeKnob (row.removeFromLeft (colW).reduced (2), envL2, decSlider);
    placeKnob (row.removeFromLeft (colW).reduced (2), envL3, susSlider);
    placeKnob (row.removeFromLeft (colW).reduced (2), envL4, relSlider);
    placeKnob (row.reduced (2),                       envL5, scaleSlider);
}

// LFO
{
    auto row = area.removeFromTop (rowH);
    secLfoLabel.setBounds (row.removeFromLeft (labelW));
    int colW = row.getWidth() / 12;
    placeCombo (row.removeFromLeft (colW).reduced (2), lfoL3,  wave1Combo);
    placeCombo (row.removeFromLeft (colW).reduced (2), lfoL4,  tgt1Combo);
    placeKnob  (row.removeFromLeft (colW).reduced (2), lfoL1,  rate1Slider);
    placeKnob  (row.removeFromLeft (colW).reduced (2), lfoL2,  amt1Slider);
    placeCombo (row.removeFromLeft (colW).reduced (2), lfoL7,  wave2Combo);
    placeCombo (row.removeFromLeft (colW).reduced (2), lfoL8,  tgt2Combo);
    placeKnob  (row.removeFromLeft (colW).reduced (2), lfoL5,  rate2Slider);
    placeKnob  (row.removeFromLeft (colW).reduced (2), lfoL6,  amt2Slider);
    placeCombo (row.removeFromLeft (colW).reduced (2), lfoL11, wave3Combo);
    placeCombo (row.removeFromLeft (colW).reduced (2), lfoL12, tgt3Combo);
    placeKnob  (row.removeFromLeft (colW).reduced (2), lfoL9,  rate3Slider);
    placeKnob  (row.reduced (2),                       lfoL10, amt3Slider);
}

// FX & MASTER
{
    auto row = area.removeFromTop (rowH);
    secFxLabel.setBounds (row.removeFromLeft (labelW));
    int colW = row.getWidth() / 5;
    placeKnob (row.removeFromLeft (colW).reduced (2), fxL1, delayTimeSlider);
    placeKnob (row.removeFromLeft (colW).reduced (2), fxL2, delayFbSlider);
    placeKnob (row.removeFromLeft (colW).reduced (2), fxL3, reverbSizeSlider);
    placeKnob (row.removeFromLeft (colW).reduced (2), fxL4, fxMixSlider);
    placeKnob (row.reduced (2),                       fxL5, masterVolSlider);
}
}
