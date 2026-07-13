#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
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
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadAudioFile (const juce::File& file, int oscIndex);

    juce::AudioProcessorValueTreeState apvts;

    // --- Coda circolare per l'oscilloscopio ---
    static constexpr int fftSize = 512;
    float visualizerBuffer[fftSize] = { 0.0f };
    int visualizerWriteIndex = 0;
    std::atomic<bool> hasNewSourceData { false };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::Synthesiser mySynth;
    const int numVoices = 8;

    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> loadedSampleBuffers[3];

    juce::dsp::StateVariableTPTFilter<float> filter1;
    juce::dsp::StateVariableTPTFilter<float> filter2;

    juce::dsp::Oscillator<float> lfo1;
    juce::dsp::Oscillator<float> lfo2;
    juce::dsp::Oscillator<float> lfo3;

    juce::dsp::DelayLine<float> delayModule;
    juce::dsp::Reverb reverbModule;
    juce::dsp::Reverb::Parameters reverbParameters;

    juce::AudioBuffer<float> delayBuffer;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessor)
};
