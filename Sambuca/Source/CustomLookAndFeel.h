#pragma once
#include <JuceHeader.h>

class SambucaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SambucaLookAndFeel()
{
    // Trova la cartella dove risiede il plugin/eseguibile corrente
    auto currentFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto assetsFolder = currentFile.getParentDirectory().getChildFile("Assets");

    // Carica le immagini direttamente dai file PNG esterni
    knobBase = juce::ImageFileFormat::loadFrom(assetsFolder.getChildFile("knob_base.png"));
    knobGlow = juce::ImageFileFormat::loadFrom(assetsFolder.getChildFile("knob_glow.png"));
}

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        // Se le immagini non sono caricate, usa il disegno di default
        if (knobBase.isNull() || knobGlow.isNull())
        {
            juce::LookAndFeel_V4::drawRotarySlider(g, x, y, width, height, sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
            return;
        }

        // Trova il centro e la dimensione corretta (60x60 nel tuo caso)
        auto radius = jmin(width / 2, height / 2);
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;

        // 1. Calcola l'angolo di rotazione corrente basato sul movimento dello slider
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // 2. DISEGNA IL KNOB BASE (Ruotato)
        g.saveState();
        // Creiamo la trasformazione: sposta al centro, ruota, rimetti a posto
        juce::AffineTransform transform = juce::AffineTransform::translation(-knobBase.getWidth() / 2.0f, -knobBase.getHeight() / 2.0f)
                                         .rotated(angle)
                                         .translated(centreX, centreY);
        
        g.drawImageTransformed(knobBase, transform);
        g.restoreState();

        // 3. DISEGNA IL GLOW (Fermo al centro, ma con opacità variabile)
        // L'opacità (da 0.0 a 1.0) segue esattamente la proporzione dello slider (sliderPosProportional)
        g.saveState();
        g.setOpacity(sliderPosProportional);
        
        // Lo disegniamo dritto sopra il knob base, scalato alla dimensione del bounding box
        g.drawImage(knobGlow, rx, ry, rw, rw, 0, 0, knobGlow.getWidth(), knobGlow.getHeight());
        g.restoreState();
    }

private:
    juce::Image knobBase;
    juce::Image knobGlow;
};
