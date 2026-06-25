#include "PluginProcessor.h"
#include "PluginEditor.h"

// Inizializzazione del costruttore includendo la creazione dei parametri APVTS
SambucaAudioProcessor::SambucaAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
        apvts(*this, nullptr, "Parameters", createParameterLayout()) // Inizializzazione APVTS
{
    formatManager.registerBasicFormats();

    mySynth.clearVoices();
    for (int i = 0; i < numVoices; ++i)
    {
        mySynth.addVoice (new SynthVoice());
    }

    mySynth.clearSounds();
    mySynth.addSound (new SynthSound());

    // Impostazione della dimensione massima di delay supportata (es. 2 secondi a 192kHz)
    delayModule.setMaximumDelayInSamples(384000);
}

SambucaAudioProcessor::~SambucaAudioProcessor() {}

// Funzione che crea e mappa l'elenco dei 34 parametri richiesti dalla plancia
juce::AudioProcessorValueTreeState::ParameterLayout SambucaAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. PARAMETRI OSCILLATORI (OSC 1, 2, 3)
    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "osc" + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Waveform", "Osc " + juce::String(i) + " Waveform", juce::StringArray{"Sine", "Saw", "Square", "Wavetable", "Sample", "Noise"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Volume", "Osc " + juce::String(i) + " Volume", 0.0f, 1.0f, 0.7f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Pitch", "Osc " + juce::String(i) + " Detune", -24.0f, 24.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "StartLoop", "Osc " + juce::String(i) + " Start/Loop", 0.0f, 1.0f, 0.0f));
    }

    // 2. PARAMETRI FILTRI EVOLUTI (Filter 1 & 2)
    for (int i = 1; i <= 2; ++i)
    {
        juce::String prefix = "filter" + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Type", "Filter " + juce::String(i) + " Type", juce::StringArray{"Lowpass", "Highpass", "Bandpass", "Notch"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Cutoff", "Filter " + juce::String(i) + " Cutoff", 20.0f, 20000.0f, 1000.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Resonance", "Filter " + juce::String(i) + " Resonance", 0.1f, 10.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Drive", "Filter " + juce::String(i) + " Drive", 1.0f, 5.0f, 1.0f));
    }

    // 3. PARAMETRI LFO (LFO 1, 2, 3)
    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "lfo" + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Rate", "LFO " + juce::String(i) + " Rate", 0.1f, 20.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Waveform", "LFO " + juce::String(i) + " Waveform", juce::StringArray{"Sine", "Triangle", "Saw", "Random"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Amount", "LFO " + juce::String(i) + " Amount", 0.0f, 1.0f, 0.0f));
    }

    // 4. PARAMETRI EFFETTI (Delay & Reverb)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time", 0.0f, 2.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Reverb Space Size", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fxMix", "FX Wet/Dry Mix", 0.0f, 1.0f, 0.2f));

    // 5. PARAMETRI GLOBALI & INVOLUPPO BREAKPOINT
    params.push_back(std::make_unique<juce::AudioParameterFloat>("wavetableMorph", "Wavetable Morphing", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envTimeScale", "Envelope Time Scale", 0.1f, 10.0f, 1.0f));

    return { params.begin(), params.end() };
}

void SambucaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    mySynth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < mySynth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(mySynth.getVoice(i)))
        {
            voice->prepareToPlay (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }

    // Configurazione e inizializzazione delle specifiche DSP per i moduli
    juce::dsp::ProcessSpec spec;
