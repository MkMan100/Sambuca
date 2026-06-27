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
#include "PluginProcessor.h" // Inclusione necessaria per mappare l'APVTS

// Struttura interna per gestire l'oscillatore ibrido
struct SambucaOscillator 
{
    enum class Mode {
        StandardWave,
        LoadedSample
    };

    Mode currentMode = Mode::StandardWave;
    
    juce::dsp::Oscillator<float> waveOsc;
    
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
    // Il costruttore ora accetta la reference del processore per accedere ai parametri APVTS
    SynthVoice (SambucaAudioProcessor& p) : processor (p)
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

        voiceLFO.prepare (spec);
        voiceFilter1.prepare (spec);

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

        // Estrazione parametri sicura dall'APVTS del processore
        auto& apvts = processor.apvts;
        
        float morphValue = apvts.getRawParameterValue("wavetableMorph")->load();
        float lfoRate = apvts.getRawParameterValue("lfo1Rate")->load();
        float lfoAmount = apvts.getRawParameterValue("lfo1Amount")->load();

        voiceLFO.setFrequency (lfoRate);

        juce::AudioBuffer<float> synthBlock (outputBuffer.getNumChannels(), numSamples);
        synthBlock.clear();

        float lastLfoValue = 0.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            lastLfoValue = voiceLFO.processSample (0.0f) * lfoAmount;

            float sampleSum = 0.0f;
            float s1 = (oscillators[0].currentMode == SambucaOscillator::Mode::StandardWave) ? oscillators[0].waveOsc.processSample (0.0f) : 0.0f;
            float s2 = (oscillators[1].currentMode == SambucaOscillator::Mode::StandardWave) ? oscillators[1].waveOsc.processSample (0.0f) : 0.0f;
            float s3 = (oscillators[2].currentMode == SambucaOscillator::Mode::StandardWave) ? oscillators[2].waveOsc.processSample (0.0f) : 0.0f;

            // Algoritmo di Morphing a 3 vie basato sullo Slider
            if (morphValue < 0.5f)
            {
                float t = morphValue * 2.0f;
                sampleSum = (1.0f - t) * s1 + t * s2;
            }
            else
            {
                float t = (morphValue - 0.5f) * 2.0f;
                sampleSum = (1.0f - t) * s2 + t * s3;
            }

            float envVolume = adsr.getNextSample();
            float finalSample = sampleSum * envVolume * noteVelocity;

            for (int channel = 0; channel < synthBlock.getNumChannels(); ++channel)
            {
                synthBlock.setSample (channel, sample, finalSample);
            }
        }

        // Configurazione Filtro Polifonico con modulazione LFO (Uso corretto di setCutoffFrequencyHz)
        float baseCutoff1 = apvts.getRawParameterValue("filter1Cutoff")->load();
        float modulatedCutoff1 = juce::jlimit (20.0f, 20000.0f, baseCutoff1 + (lastLfoValue * 5000.0f));
        
        voiceFilter1.setCutoffFrequencyHz (modulatedCutoff1);
        voiceFilter1.setResonance (apvts.getRawParameterValue("filter1Resonance")->load());

        juce::dsp::AudioBlock<float> block (synthBlock);
        juce::dsp::ProcessContextReplacing<float> context (block);
        voiceFilter1.process (context);

        // Somma il blocco processato al buffer audio di uscita globale
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addFrom (channel, startSample, synthBlock.getReadPointer(channel), numSamples);
        }

        if (!adsr.isActive())
        {
            clearCurrentNote();
        }
    }

private:
    SambucaAudioProcessor& processor; // Reference al processore
    SambucaOscillator oscillators[3];
    juce::ADSR adsr;
    float noteVelocity = 0.0f;

    juce::dsp::Oscillator<float> voiceLFO;
    juce::dsp::LadderFilter<float> voiceFilter1;
};
