#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

// Definizione delle classi di supporto per il synth
class SynthSound : public juce::SynthesiserSound
{
public:
    SynthSound() {}
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice (juce::AudioProcessorValueTreeState& state) : apvts (state) {}
    bool canPlaySound (juce::SynthesiserSound*) override { return true; }
    void startNote (int, float, juce::SynthesiserSound*, int) override {}
    void stopNote (float, bool) override {}
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int, int) override {}
    void prepareToPlay (double, int, int) {}
    void setSampleBufferPointer (int, juce::AudioBuffer<float>*) {}
private:
    juce::AudioProcessorValueTreeState& apvts;
};

// ==============================================================================
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

    // Metodo dichiarato esattamente come implementato nel tuo cpp
    void loadAudioFile (const juce::File& file, int oscIndex);

    juce::AudioProcessorValueTreeState apvts;

    // Gestione dell'oscilloscopio
    static constexpr int fftSize = 1024;
    float visualizerBuffer[fftSize] = { 0.0f };
    int visualizerWriteIndex = 0;
    std::atomic<bool> hasNewSourceData { false };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double currentSampleRate = 44100.0;
    static constexpr int numVoices = 8;

    juce::Synthesiser mySynth;
    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> loadedSampleBuffers[3];

    juce::dsp::StateVariableTPTFilter<float> filter1, filter2;
    juce::dsp::Oscillator<float> lfo1, lfo2, lfo3;
    juce::dsp::DelayLine<float> delayModule;
    juce::dsp::Reverb reverbModule;
    juce::Reverb::Parameters reverbParameters;
    juce::AudioBuffer<float> delayBuffer;

    JUDER_MEMBER_POINTERS;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SambucaAudioProcessor)
};
