#include "PluginProcessor.h"
#include "PluginEditor.h"

// Funzione di utilità interna per scrivere i log sul Desktop senza bloccare i thread
void writeDebugLog(const juce::String& message)
{
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("sambuca_debug.txt");
    logFile.appendText(message + "\n");
}

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
        apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // 1. Reset del file di log
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("sambuca_debug.txt");
    logFile.deleteFile();

    writeDebugLog("--- LOG AVVIO SAMBUCA ---");
    writeDebugLog("[Costruttore] Inizio inizializzazione");

    formatManager.registerBasicFormats();

    // 2. Inizializzazione di sicurezza dei 3 buffer (Stereo, 100 campioni di silenzio)
    for (int i = 0; i < 3; ++i)
    {
        loadedSampleBuffers[i].setSize (2, 100);
        loadedSampleBuffers[i].clear();
    }

    // 3. Creazione delle voci del sintetizzatore passando *this (Risolve l'errore C2065/C2530)
    mySynth.clearVoices();
    for (int i = 0; i < numVoices; ++i)
    {
        mySynth.addVoice (new SynthVoice (*this)); 
    }

    mySynth.clearSounds();
    mySynth.addSound (new SynthSound());

    delayModule.setMaximumDelayInSamples(384000);

    writeDebugLog("[Costruttore] Fine inizializzazione riuscita");
}

SambucaAudioProcessor::~SambucaAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout SambucaAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "osc" + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Waveform", "Osc " + juce::String(i) + " Waveform", juce::StringArray{"Sine", "Saw", "Square", "Wavetable", "Sample", "Noise"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Volume", "Osc " + juce::String(i) + " Volume", 0.0f, 1.0f, 0.7f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Pitch", "Osc " + juce::String(i) + " Detune", -24.0f, 24.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "StartLoop", "Osc " + juce::String(i) + " Start/Loop", 0.0f, 1.0f, 0.0f));
    }

    for (int i = 1; i <= 2; ++i)
    {
        juce::String prefix = "filter" + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Type", "Filter " + juce::String(i) + " Type", juce::StringArray{"Lowpass", "Highpass", "Bandpass", "Notch"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Cutoff", "Filter " + juce::String(i) + " Cutoff", 20.0f, 20000.0f, 1000.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Resonance", "Filter " + juce::String(i) + " Resonance", 0.1f, 10.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Drive", "Filter " + juce::String(i) + " Drive", 1.0f, 5.0f, 1.0f));
    }

    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "lfo" + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Rate", "LFO " + juce::String(i) + " Rate", 0.1f, 20.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Waveform", "LFO " + juce::String(i) + " Waveform", juce::StringArray{"Sine", "Triangle", "Saw", "Random"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Amount", "LFO " + juce::String(i) + " Amount", 0.0f, 1.0f, 0.5f));
    }

    // Iniezione Parametri ADSR (Evita i log di errore nell'Editor)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("attack", "Attack", 0.001f, 5.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", 0.01f, 5.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("release", "Release", 0.01f, 5.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time", 0.0f, 2.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Reverb Space Size", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fxMix", "FX Wet/Dry Mix", 0.0f, 1.0f, 0.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("wavetableMorph", "Wavetable Morphing", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envTimeScale", "Envelope Time Scale", 0.1f, 10.0f, 1.0f));

    return { params.begin(), params.end() };
}

void SambucaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    writeDebugLog("[prepareToPlay] Configurazione motore DSP avviata");
    currentSampleRate = sampleRate;
    mySynth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < mySynth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(mySynth.getVoice(i)))
        {
            voice->prepareToPlay (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    filter1.prepare(spec);
    filter2.prepare(spec);
    
    lfo1.prepare(spec);
    lfo2.prepare(spec);
    lfo3.prepare(spec);
    
    lfo1.initialise([](float x) { return std::sin(x); });
    lfo2.initialise([](float x) { return std::sin(x); });
    lfo3.initialise([](float x) { return std::sin(x); });

    delayModule.prepare(spec);
    reverbModule.prepare(spec);
    
    delayBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    writeDebugLog("[prepareToPlay] Configurazione completata con successo");
}

void SambucaAudioProcessor::releaseResources() {}

void SambucaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    static bool hasLoggedProcess = false;
    if (!hasLoggedProcess) {
        writeDebugLog("[processBlock] Entrato nel ciclo audio principale");
        hasLoggedProcess = true;
    }

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    mySynth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Gestione Effetti (Delay con Loop di Feedback attivo)
    auto* fxMixPtr = apvts.getRawParameterValue("fxMix");
    float fxMix = (fxMixPtr != nullptr) ? fxMixPtr->load() : 0.2f;
    fxMix = juce::jlimit(0.0f, 1.0f, fxMix);

    if (fxMix > 0.0f)
    {
        delayBuffer.makeCopyOf(buffer); 
        juce::dsp::AudioBlock<float> delayBlock(delayBuffer);
        juce::dsp::ProcessContextReplacing<float> delayContext(delayBlock);

        auto* delayTimePtr = apvts.getRawParameterValue("delayTime");
        float delaySecs = (delayTimePtr != nullptr) ? delayTimePtr->load() : 0.3f;
        
        if (currentSampleRate > 0)
        {
            delayModule.setDelay(juce::jlimit(0.0f, 2.0f, delaySecs) * currentSampleRate);
        }

        auto* feedbackPtr = apvts.getRawParameterValue("delayFeedback");
        float feedback = (feedbackPtr != nullptr) ? juce::jlimit(0.0f, 0.95f, feedbackPtr->load()) : 0.5f;

        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            delayModule.pushSample(ch, delayBuffer.getReadPointer(ch)[0] * feedback);
        }

        delayModule.process(delayContext);

        auto* reverbSizePtr = apvts.getRawParameterValue("reverbSize");
        reverbParameters.roomSize = (reverbSizePtr != nullptr) ? juce::jlimit(0.0f, 1.0f, reverbSizePtr->load()) : 0.5f;
        reverbParameters.wetLevel = fxMix;
        reverbParameters.dryLevel = 1.0f - fxMix;
        reverbModule.setParameters(reverbParameters);
        reverbModule.process(delayContext);

        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            buffer.addFrom(ch, 0, delayBuffer.getReadPointer(ch), buffer.getNumSamples(), fxMix);
        }
    }

    auto* masterGainPtr = apvts.getRawParameterValue("masterVolume");
    float masterGain = (masterGainPtr != nullptr) ? masterGainPtr->load() : 0.8f;
    buffer.applyGain(juce::jlimit(0.0f, 1.0f, masterGain));
}

juce::AudioProcessorEditor* SambucaAudioProcessor::createEditor()
{
    return new SambucaAudioProcessorEditor (*this);
}

void SambucaAudioProcessor::loadAudioFile (const juce::File& file, int oscillatorIndex)
{
    if (oscillatorIndex < 0 || oscillatorIndex >= 3) return;

    auto* reader = formatManager.createReaderFor (file);
    if (reader != nullptr)
    {
        loadedSampleBuffers[oscillatorIndex].setSize (1, static_cast<int>(reader->lengthInSamples));
        reader->read (&loadedSampleBuffers[oscillatorIndex], 0, static_cast<int>(reader->lengthInSamples), 0, true, false);
        delete reader;
    }
}

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
