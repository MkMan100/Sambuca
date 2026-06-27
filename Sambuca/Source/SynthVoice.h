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

        juce::AudioBuffer<float> synthBlock (outputBuffer.getNumChannels(), numSamples);
        synthBlock.clear();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float sampleSum = 0.0f;

            for (int i = 0; i < 3; ++i)
            {
                if (oscillators[i].currentMode == SambucaOscillator::Mode::StandardWave)
                {
                    sampleSum += oscillators[i].waveOsc.processSample (0.0f);
                }
                else if (oscillators[i].currentMode == SambucaOscillator::Mode::LoadedSample && oscillators[i].sampleBufferRef != nullptr)
                {
                    auto& buffer = *(oscillators[i].sampleBufferRef);
                    int bufferLength = buffer.getNumSamples();
                    int numChannels = buffer.getNumChannels();
                    
                    if (bufferLength <= 1 || numChannels <= 0)
                    {
                        sampleSum += 0.0f;
                    }
                    else
                    {
                        int idxCurrent = static_cast<int>(oscillators[i].samplePosition);
                        int idxNext = (idxCurrent + 1) % bufferLength;
                        float fraction = oscillators[i].samplePosition - idxCurrent;

                        float s0 = buffer.getSample (0, idxCurrent);
                        float s1 = buffer.getSample (0, idxNext);
                        float interpolatedSample = s0 + fraction * (s1 - s0);

                        sampleSum += interpolatedSample;

                        oscillators[i].samplePosition += oscillators[i].sampleIncrement;
                        if (oscillators[i].samplePosition >= bufferLength)
                        {
                            if (oscillators[i].isLooping)
                                oscillators[i].samplePosition = std::fmod(oscillators[i].samplePosition, (double)bufferLength);
                            else
                                oscillators[i].sampleIncrement = 0.0;
                        }
                    }
                }
            }

            float envVolume = adsr.getNextSample();
            float finalSample = (sampleSum / 3.0f) * envVolume * noteVelocity;

            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            {
                outputBuffer.addSample (channel, startSample + sample, finalSample);
            }
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
};
