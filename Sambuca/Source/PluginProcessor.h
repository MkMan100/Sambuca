#pragma once

// Sostituiamo <JuceHeader.h> con le inclusioni dirette dei moduli JUCE richiesti
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

class SynthVoice;
class SynthSound;

class SambucaAudioProcessor  : public juce::AudioProcessor
{
public:
    // ==============================================================================
    SambucaAudioProcessor();
    ~SambucaAudioProcessor() override;

    // ==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // ==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // ==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // ==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    // ==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Metodi di caricamento audio richiamati dall'Editor
    void loadAudioFile (const juce::File& file, int oscIndex);
    void loadSample (int oscIndex, const juce::File& file);

    // Layout dei parametri
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Variabili per l'oscilloscopio accessibili dall'Editor
    static constexpr int fftSize = 1024;
    float visualizerBuffer[fftSize] = { 0.0f };
    int visualizerWriteIndex = 0;
    bool hasNewSourceData = false;

private:
    // Sintetizzatore e Voci
    juce::Synthesiser mySynth;
    static constexpr int numVoices = 8;

    // Buffer dei sample per gli oscillatori
    juce::AudioBuffer<float> loadedSampleBuffers[3];
    juce::AudioFormatManager formatManager;

    // DSP Engine
    double currentSampleRate = 44100.0;
    
    juce::dsp::StateVariableTPTFilter<float> filter1;
    juce::dsp::StateVariableTPTFilter<float> filter2;

    juce::dsp::Oscillator<float> lfo1;
    juce::dsp::Oscillator<float> lfo2;
    juce::dsp::Oscillator<float> lfo3;

    juce::dsp::DelayLine<float> delayModule;
    juce::AudioBuffer<float> delayBuffer;

    juce::dsp::Reverb reverbModule;
    juce::dsp::Reverb::Parameters reverbParameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessor)
};
