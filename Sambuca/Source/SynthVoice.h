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
            // Pitch shifting: rapportiamo la frequenza desiderata con una frequenza base stimata (es. C3 = 261.63 Hz)
            float baseFrequency = 261.63f; 
            double pitchRatio = frequency / baseFrequency;
            sampleIncrement = pitchRatio * (44100.0 / hostSampleRate); // adatta il sample rate
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
        // Impostiamo l'onda iniziale standard (es. Dente di sega) per i 3 oscillatori
        for (int i = 0; i < 3; ++i)
        {
            oscillators[i].waveOsc.initialise ([] (float x) { return std::sin (x); }); // Partiamo con una sinusoide pura
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
            oscillators[i].samplePosition = 0.0; // Resetta la lettura del sample dall'inizio
        }
        
        // Attiva l'inviluppo quando premiamo la nota
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
        
        // Impostazioni ADSR temporanee di base prima di fare quelle Multi-Breakpoint
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

        // Creiamo un buffer temporaneo per sommare i 3 oscillatori prima dell'inviluppo
        juce::AudioBuffer<float> synthBlock (outputBuffer.getNumChannels(), numSamples);
        synthBlock.clear();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float sampleSum = 0.0f;

            for (int i = 0; i < 3; ++i)
            {
                if (oscillators[i].currentMode == SambucaOscillator::Mode::StandardWave)
                {
                    // Calcolo matematico classico
                    sampleSum += oscillators[i].waveOsc.processSample (0.0f);
                }
                else if (oscillators[i].currentMode == SambucaOscillator::Mode::LoadedSample && oscillators[i].sampleBufferRef != nullptr)
                {
                    // Lettura e interpolazione del file .wav caricato in RAM
                    auto& buffer = *(oscillators[i].sampleBufferRef);
                    int bufferLength = buffer.getNumSamples();
                    
                    int idxCurrent = static_cast<int>(oscillators[i].samplePosition);
                    int idxNext = (idxCurrent + 1) % bufferLength;
                    float fraction = oscillators[i].samplePosition - idxCurrent;

                    // Interpolazione lineare per evitare distorsioni e "click"
                    float s0 = buffer.getSample (0, idxCurrent);
                    float s1 = buffer.getSample (0, idxNext);
                    float interpolatedSample = s0 + fraction * (s1 - s0);

                    sampleSum += interpolatedSample;

                    // Avanza l'indice di lettura
                    oscillators[i].samplePosition += oscillators[i].sampleIncrement;
                    if (oscillators[i].samplePosition >= bufferLength)
                    {
                        if (oscillators[i].isLooping)
                            oscillators[i].samplePosition = std::fmod(oscillators[i].samplePosition, (double)bufferLength);
                        else
                            oscillators[i].sampleIncrement = 0.0; // si ferma a fine file
                    }
                }
            }

            // Applichiamo l'inviluppo ADSR generale e la velocity della nota premuta
            float envVolume = adsr.getNextSample();
            float finalSample = (sampleSum / 3.0f) * envVolume * noteVelocity;

            // Scriviamo il risultato su tutti i canali d'uscita (Stereo)
            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            {
                outputBuffer.addSample (channel, startSample + sample, finalSample);
            }
        }

        // Se l'inviluppo si è spento del tutto, spegni la voce del synth liberando CPU
        if (!adsr.isActive())
        {
            clearCurrentNote();
        }
    }

private:
    SambucaOscillator oscillators[3]; // I nostri 3 moduli pronti ad accogliere onde o file audio
    juce::ADSR adsr;
    float noteVelocity = 0.0f;
};
