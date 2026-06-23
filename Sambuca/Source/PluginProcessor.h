#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h> // Incluso per i filtri ed effetti dsp
#include "SynthVoice.h"

class SambucaAudioProcessor  : public juce::AudioProcessor
{
public:
    SambucaAudioProcessor();
    ~SambucaAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Metodo pubblico per permettere all'Editor (GUI) di caricare un file audio
    void loadAudioFile (const juce::File& file);

    // Funzione helper obbligatoria per inizializzare l'albero dei parametri (APVTS)
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Oggetto pubblico APVTS a cui l'Editor si collegherà per muovere i Knob
    juce::AudioProcessorValueTreeState apvts;

private:
    juce::Synthesiser mySynth;
    const int numVoices = 8; 

    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> loadedSampleBuffer;

    // --- NUOVI COMPONENTI DSP (Stile Absynth) ---
    
    // 2 Filtri di stato variabili (SVF) in cascata
    juce::dsp::StateVariableTPTFilter<float> filter1;
    juce::dsp::StateVariableTPTFilter<float> filter2;

    // 3 LFO Generici basati su oscillatori DSP
    juce::dsp::Oscillator<float> lfo1;
    juce::dsp::Oscillator<float> lfo2;
    juce::dsp::Oscillator<float> lfo3;

    // Effetti di Spazio/Tempo
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayModule;
    juce::dsp::Reverb reverbModule;
    juce::dsp::Reverb::Parameters reverbParameters;

    // Variabili di supporto per la conversione e il campionamento dell'effetto Delay
    double currentSampleRate = 44100.0;
    juce::AudioBuffer<float> delayBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessor)
};