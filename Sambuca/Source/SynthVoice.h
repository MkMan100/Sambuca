#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// 1. Diciamo solo che la classe esiste
class SambucaAudioProcessor; 

// 2. Struttura dell'oscillatore
struct SambucaOscillator 
{
    enum class Mode { StandardWave, LoadedSample };
    Mode currentMode = Mode::StandardWave;
    juce::dsp::Oscillator<float> waveOsc;
    juce::AudioBuffer<float>* sampleBufferRef = nullptr;
    double samplePosition = 0.0;
    double sampleIncrement = 0.0;
    bool isLooping = true;

    void prepare (const juce::dsp::ProcessSpec& spec) { waveOsc.prepare (spec); }
    void setFrequency (float frequency, double hostSampleRate)
    {
        if (currentMode == Mode::StandardWave) waveOsc.setFrequency (frequency);
    }
};

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

// 3. Definizione della Classe della Voce (Solo dichiarazioni, niente corpi funzione complessi che usano 'processor')
class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice (SambucaAudioProcessor& p); // Implementato sotto

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<SynthSound*>(sound) != nullptr; }
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    SambucaAudioProcessor& processor; 
    SambucaOscillator oscillators[3];
    juce::ADSR adsr;
    float noteVelocity = 0.0f;
    juce::dsp::Oscillator<float> voiceLFO;
    juce::dsp::LadderFilter<float> voiceFilter1;
};

// ==============================================================================
// 4. ORA INCLUDIAMO IL PROCESSORE (Adesso che SynthVoice è definita, non c'è loop!)
// ==============================================================================
#include "PluginProcessor.h"

// 5. IMPLEMENTAZIONE DEI METODI DI SYNTHVOICE
// Ora il compilatore sa ESATTAMENTE cos'è 'processor' e cos'è 'apvts'

inline SynthVoice::SynthVoice (SambucaAudioProcessor& p) : processor (p)
{
    for (int i = 0; i < 3; ++i)
        oscillators[i].waveOsc.initialise ([] (float x) { return std::sin (x); });
}

inline void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
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

inline void SynthVoice::stopNote (float, bool allowTailOff)
{
    adsr.noteOff();
    if (!allowTailOff || !adsr.isActive()) clearCurrentNote();
}

inline void SynthVoice::prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), static_cast<juce::uint32> (outputChannels) };
    for (int i = 0; i < 3; ++i) oscillators[i].prepare (spec);
    voiceLFO.prepare (spec);
    voiceFilter1.prepare (spec);
    adsr.setSampleRate (sampleRate);
}

inline void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive()) return;

    // Compila perfettamente perché PluginProcessor.h è stato incluso poche righe sopra!
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

        float voiceSample = sampleSum * adsr.getNextSample() * noteVelocity;
        for (int channel = 0; channel < synthBlock.getNumChannels(); ++channel)
            synthBlock.setSample (channel, sample, voiceSample);
    }

    float baseCutoff1 = apvts.getRawParameterValue("filter1Cutoff")->load();
    float modulatedCutoff1 = juce::jlimit (20.0f, 20000.0f, baseCutoff1 + (lastLfoValue * 5000.0f));
    
    voiceFilter1.setCutoffFrequencyHz (modulatedCutoff1);
    voiceFilter1.setResonance (apvts.getRawParameterValue("filter1Resonance")->load());

    juce::dsp::AudioBlock<float> block (synthBlock);
    juce::dsp::ProcessContextReplacing<float> context (block);
    voiceFilter1.process (context);

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        outputBuffer.addFrom (channel, startSample, synthBlock.getReadPointer(channel), numSamples);

    if (!adsr.isActive()) clearCurrentNote();
}
