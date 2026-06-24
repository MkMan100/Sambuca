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

// Funzione enorme che crea e mappa l'elenco dei 34 parametri richiesti dalla plancia
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
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    filter1.prepare(spec);
    filter2.prepare(spec);
    
    lfo1.prepare(spec);
    lfo2.prepare(spec);
    lfo3.prepare(spec);
    
    // Assegnazione iniziale delle forme d'onda dell'LFO (Funzioni Lambda)
    lfo1.initialise([](float x) { return std::sin(x); });
    lfo2.initialise([](float x) { return std::sin(x); });
    lfo3.initialise([](float x) { return std::sin(x); });

    delayModule.prepare(spec);
    reverbModule.prepare(spec);
    
    delayBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
}

void SambucaAudioProcessor::releaseResources() {}

void SambucaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 1. Generazione del suono base dai 3 oscillatori della Synthesiser
    mySynth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Configurazione del blocco DSP per l'elaborazione successiva
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // 2. AGGIORNAMENTO LIVE PARAMETRI FILTRI ED ELABORAZIONE IN CASCATA
    auto updateFilter = [](auto& filter, std::atomic<float>* typePtr, std::atomic<float>* cutoffPtr, std::atomic<float>* resPtr, std::atomic<float>* drivePtr) 
    {
        if (typePtr == nullptr || cutoffPtr == nullptr || resPtr == nullptr) return;

        int typeIdx = static_cast<int>(typePtr->load());
        float cutoff = cutoffPtr->load();
        float res = resPtr->load();

        // Mappa la scelta del tipo filtro all'algoritmo SVF specifico (Notch con la N maiuscola!)
        if (typeIdx == 0) filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        else if (typeIdx == 1) filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        else if (typeIdx == 2) filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        else filter.setType(juce::dsp::StateVariableTPTFilterType::Notch); 
        
        filter.setCutoffFrequency(cutoff);
        filter.setResonance(res);
    };
    updateFilter(filter1, apvts.getRawParameterValue("filter1Type"), apvts.getRawParameterValue("filter1Cutoff"), apvts.getRawParameterValue("filter1Resonance"), apvts.getRawParameterValue("filter1Drive"));
    updateFilter(filter2, apvts.getRawParameterValue("filter2Type"), apvts.getRawParameterValue("filter2Cutoff"), apvts.getRawParameterValue("filter2Resonance"), apvts.getRawParameterValue("filter2Drive"));

    // Applica Filtro 1 e poi Filtro 2 in cascata (Serie)
    filter1.process(context);
    filter2.process(context);

    // 3. SEZIONE EFFETTI SPAZIALI (Delay & Reverb in Mandata / Mix)
    auto* fxMixPtr = apvts.getRawParameterValue("fxMix");
    float fxMix = (fxMixPtr != nullptr) ? fxMixPtr->load() : 0.2f;

    if (fxMix > 0.0f)
    {
        delayBuffer.makeCopyOf(buffer); // Copia segnale dry per lavorazione FX
        juce::dsp::AudioBlock<float> delayBlock(delayBuffer);
        juce::dsp::ProcessContextReplacing<float> delayContext(delayBlock);

        // Imposta il tempo del delay convertendo i secondi in campioni
        auto* delayTimePtr = apvts.getRawParameterValue("delayTime");
        float delaySecs = (delayTimePtr != nullptr) ? delayTimePtr->load() : 0.3f;
        delayModule.setDelay(delaySecs * currentSampleRate);
        delayModule.process(delayContext);

        // Aggiorna parametri riverbero
        auto* reverbSizePtr = apvts.getRawParameterValue("reverbSize");
        reverbParameters.roomSize = (reverbSizePtr != nullptr) ? reverbSizePtr->load() : 0.5f;
        reverbParameters.wetLevel = fxMix;
        reverbParameters.dryLevel = 1.0f - fxMix;
        reverbModule.setParameters(reverbParameters);
        reverbModule.process(delayContext);

        // Miscela il segnale effettato (Wet) dentro il buffer audio finale
        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            buffer.addFrom(ch, 0, delayBuffer.getReadPointer(ch), buffer.getNumSamples(), fxMix);
        }
    }

    // 4. REGOLAZIONE VOLUME MASTER FINALE
    auto* masterGainPtr = apvts.getRawParameterValue("masterVolume");
    float masterGain = (masterGainPtr != nullptr) ? masterGainPtr->load() : 0.8f;
    buffer.applyGain(masterGain);
}

juce::AudioProcessorEditor* SambucaAudioProcessor::createEditor()
{
    return new SambucaAudioProcessorEditor (*this);
}

void SambucaAudioProcessor::loadAudioFile (const juce::File& file)
{
    auto* reader = formatManager.createReaderFor (file);
    
    if (reader != nullptr)
    {
        loadedSampleBuffer.setSize (1, static_cast<int>(reader->lengthInSamples));
        reader->read (&loadedSampleBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, false);
        
        for (int i = 0; i < mySynth.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<SynthVoice*>(mySynth.getVoice(i)))
            {
                // In futuro collegheremo il buffer dinamicamente tramite APVTS o puntatori diretti
                // voice->setSampleBufferForOsc (0, &loadedSampleBuffer); 
            }
        }
        
        delete reader;
    }
}

// Gestione del salvataggio dello stato dei parametri (per salvare i preset nella DAW)
void SambucaAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SambucaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SambucaAudioProcessor();
}
