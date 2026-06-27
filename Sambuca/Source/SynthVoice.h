/*
  ==============================================================================

    SynthVoice.h
    Created: 21 Jun 2026 6:25:39pm
    Author:  MkMan

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// Struttura interna per gestire l'oscillatore ibrido
struct SambucaOscillator 
{
    enum class Mode {
        StandardWave,
        LoadedSample
    };

    Mode currentMode = Mode::StandardWave;
    
    // 1. Componente per le onde matematiche standard
    juce::dsp::Oscillator<float> waveOsc;
    
    // 2. Puntatori per la lettura del file .wav caricato in RAM
    juce::AudioBuffer<float>* sampleBufferRef = nullptr;
    double samplePosition = 0.0;
    double sampleIncrement = 0.0;
    bool isLooping = true;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        waveOsc.prepare (spec);
    }

    void setFrequency (float frequency, double hostSampleRate)
    {
        if (currentMode == Mode::StandardWave)
        {
            waveOsc.setFrequency (frequency);
        }
        else if (currentMode == Mode::LoadedSample && sampleBufferRef != nullptr)
        {
            float baseFrequency = 261.63f; 
            double pitchRatio = frequency / baseFrequency;
            sampleIncrement = pitchRatio * (44100.0 / hostSampleRate);
        }
    }
};

// Classi standard di JUCE per agganciare il suono polifonico
class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice()
    {
        for (int i = 0; i < 3; ++i)
        {
            oscillators[i].waveOsc.initialise ([] (float x) { return std::sin (x); });
        }
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*>(sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override
    {
        noteVelocity = velocity;
        auto frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        
        for (int i = 0; i < 3; ++i)
        {
            oscillators[i].setFrequency (frequency, getSampleRate());
            oscillators[i].samplePosition = 0.0;
        }
        
        adsr.noteOn();
    }

    void stopNote (float velocity, bool allowTailOff) override
    {
        adsr.noteOff();
        
        if (!allowTailOff || !adsr.isActive())
            clearCurrentNote();
    }

    void pitchWheelMoved (int newPitchWheelValue) override {}
    void controllerMoved (int controllerNumber, int newControllerValue) override {}

    void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = outputChannels;

        for (int i = 0; i < 3; ++i)
        {
            oscillators[i].prepare (spec);
        }

        adsr.setSampleRate (sampleRate);
        
        juce::ADSR::Parameters adsrParams;
        adsrParams.attack  = 0.1f;
        adsrParams.decay   = 0.2f;
        adsrParams.sustain = 0.8f;
        adsrParams.release = 0.5f;
        adsr.setParameters (adsrParams);
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
{
    if (!isVoiceActive())
        return;

    // Recuperiamo i puntatori ai parametri APVTS (tramite il processore)
    // Nota: Assicurati che la tua voce abbia un modo per accedere all'APVTS del processore,
    // ad esempio passando un puntatore al processore nel costruttore o tramite una funzione.
    auto& apvts = audioProcessor.apvts; 

    // Leggi i valori aggiornati dall'interfaccia
    float morphValue = apvts.getRawParameterValue("wavetableMorph")->load(); // Problema 1 & 3 (Morphing)
    float lfoRate = apvts.getRawParameterValue("lfo1Rate")->load();
    float lfoAmount = apvts.getRawParameterValue("lfo1Amount")->load();

    // Aggiorna la frequenza dell'LFO di questa voce
    voiceLFO.setFrequency(lfoRate);

    juce::AudioBuffer<float> synthBlock (outputBuffer.getNumChannels(), numSamples);
    synthBlock.clear();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Calcola il campione corrente dell'LFO (Problema 6)
        float lfoCurrentValue = voiceLFO.processSample(0.0f) * lfoAmount;

        float sampleSum = 0.0f;

        // Calcolo individuale degli oscillatori
        float s1 = oscillators[0].currentMode == SambucaOscillator::Mode::StandardWave ? oscillators[0].waveOsc.processSample (0.0f) : 0.0f; // Esegui logica WAV se LoadedSample
        float s2 = oscillators[1].currentMode == SambucaOscillator::Mode::StandardWave ? oscillators[1].waveOsc.processSample (0.0f) : 0.0f;
        float s3 = oscillators[2].currentMode == SambucaOscillator::Mode::StandardWave ? oscillators[2].waveOsc.processSample (0.0f) : 0.0f;

        // --- APPLICAZIONE MORPHING (Problema 1 & 3) ---
        // Il parametro morphValue (0.0 - 1.0) miscela fluidamente tra OSC 1, OSC 2 e OSC 3
        if (morphValue < 0.5f)
        {
            float t = morphValue * 2.0f; // riscala 0.0 - 0.5 in 0.0 - 1.0
            sampleSum = (1.0f - t) * s1 + t * s2;
        }
        else
        {
            float t = (morphValue - 0.5f) * 2.0f; // riscala 0.5 - 1.0 in 0.0 - 1.0
            sampleSum = (1.0f - t) * s2 + t * s3;
        }

        // --- APPLICAZIONE ADSR (Problema 2) ---
        float envVolume = adsr.getNextSample();
        
        // Salva il segnale combinato temporaneamente nel blocco della voce
        float voiceSample = sampleSum * envVolume * noteVelocity;

        for (int channel = 0; channel < synthBlock.getNumChannels(); ++channel)
        {
            synthBlock.setSample(channel, sample, voiceSample);
        }
    }

    // --- APPLICAZIONE FILTRI POLIFONICI (Problema 5) ---
    // Aggiorna i filtri con l'aggiunta della modulazione LFO sul Cutoff prima del processamento
    float baseCutoff1 = apvts.getRawParameterValue("filter1Cutoff")->load();
    float modulatedCutoff1 = juce::jlimit(20.0f, 20000.0f, baseCutoff1 + (lfoCurrentValue * 5000.0f)); // LFO modula fino a 5kHz
    
    voiceFilter1.setCutoffFrequency(modulatedCutoff1);
    voiceFilter1.setResonance(apvts.getRawParameterValue("filter1Resonance")->load());

    juce::dsp::AudioBlock<float> block(synthBlock);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    // Il filtro elabora il blocco audio di questa specifica nota
    voiceFilter1.process(context);

    // Copia il risultato finale nell'output generale del synth
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        outputBuffer.addFrom(channel, startSample, synthBlock.getReadPointer(channel), numSamples);
    }

    if (!adsr.isActive())
    {
        clearCurrentNote();
    }
}

private:
    SambucaOscillator oscillators[3];
    juce::ADSR adsr;
    float noteVelocity = 0.0f;
  
    juce::dsp::Oscillator<float> voiceLFO;
    juce::dsp::LadderFilter<float> voiceFilter1;
    juce::dsp::LadderFilter<float> voiceFilter2;
};
