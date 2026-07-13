#include "PluginProcessor.h"
#include "PluginEditor.h"

void writeDebugLog(const juce::String& message)
{
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("sambuca_debug.txt");
    logFile.appendText(message + "\n");
}

SambucaAudioProcessor::SambucaAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < 3; ++i)
    {
        loadedSampleBuffers[i].setSize (2, 100);
        loadedSampleBuffers[i].clear();
    }

    mySynth.clearVoices();
    for (int i = 0; i < numVoices; ++i)
    {
        auto* voice = new SynthVoice (apvts);
        for (int oscIdx = 0; oscIdx < 3; ++oscIdx)
        {
            voice->setSampleBufferPointer (oscIdx, &loadedSampleBuffers[oscIdx]);
        }
        mySynth.addVoice (voice); 
    }

    mySynth.clearSounds();
    mySynth.addSound (new SynthSound());
    delayModule.setMaximumDelayInSamples(384000);
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
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Type", "Filter " + juce::String(i) + " Type", juce::StringArray{"Lowpass", "Highpass", "Bandpass"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Cutoff", "Filter " + juce::String(i) + " Cutoff", 20.0f, 20000.0f, 1000.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Resonance", "Filter " + juce::String(i) + " Resonance", 0.1f, 3.5f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Drive", "Filter " + juce::String(i) + " Drive", 1.0f, 5.0f, 1.0f));
    }

    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "lfo" + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Rate", "LFO " + juce::String(i) + " Rate", 0.1f, 20.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Waveform", "LFO " + juce::String(i) + " Waveform", juce::StringArray{"Sine", "Triangle", "Saw", "Random"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "Amount", "LFO " + juce::String(i) + " Amount", 0.0f, 1.0f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "Target", "LFO " + juce::String(i) + " Target", juce::StringArray{"None", "Filter Cutoff", "Osc Volume", "Osc Pitch"}, 0));
    }

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

void SambucaAudioProcessor::loadAudioFile (const juce::File& file, int oscIndex)
{
    if (oscIndex < 0 || oscIndex >= 3) return;
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader != nullptr)
    {
        loadedSampleBuffers[oscIndex].setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&loadedSampleBuffers[oscIndex], 0, (int) reader->lengthInSamples, 0, true, true);
    }
}

void SambucaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

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
}

void SambucaAudioProcessor::releaseResources() {}

void SambucaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    mySynth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    auto numSamples = buffer.getNumSamples();

    auto type1 = static_cast<int>(apvts.getRawParameterValue("filter1Type")->load());
    auto cutoff1 = apvts.getRawParameterValue("filter1Cutoff")->load();
    auto res1 = apvts.getRawParameterValue("filter1Resonance")->load();
    auto drive1 = apvts.getRawParameterValue("filter1Drive")->load();

    auto type2 = static_cast<int>(apvts.getRawParameterValue("filter2Type")->load());
    auto cutoff2 = apvts.getRawParameterValue("filter2Cutoff")->load();
    auto res2 = apvts.getRawParameterValue("filter2Resonance")->load();
    auto drive2 = apvts.getRawParameterValue("filter2Drive")->load();

    float safeRes1 = juce::jlimit(0.1f, 2.5f / (drive1 * 0.5f + 1.0f), res1);
    float safeRes2 = juce::jlimit(0.1f, 2.5f / (drive2 * 0.5f + 1.0f), res2);

    switch (type1)
    {
        case 0:  filter1.setType (juce::dsp::StateVariableTPTFilterType::lowpass);  break;
        case 1:  filter1.setType (juce::dsp::StateVariableTPTFilterType::highpass); break;
        case 2:  filter1.setType (juce::dsp::StateVariableTPTFilterType::bandpass); break;
        default: filter1.setType (juce::dsp::StateVariableTPTFilterType::lowpass);  break;
    }
    filter1.setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, cutoff1));
    filter1.setResonance(safeRes1);

    switch (type2)
    {
        case 0:  filter2.setType (juce::dsp::StateVariableTPTFilterType::lowpass);  break;
        case 1:  filter2.setType (juce::dsp::StateVariableTPTFilterType::highpass); break;
        case 2:  filter2.setType (juce::dsp::StateVariableTPTFilterType::bandpass); break;
        default: filter2.setType (juce::dsp::StateVariableTPTFilterType::lowpass);  break;
    }
    filter2.setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, cutoff2));
    filter2.setResonance(safeRes2);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            float x = channelData[sample];

            x = filter1.processSample(ch, x);

            if (drive1 > 1.0f)
            {
                x *= drive1;
                if (x > 0.0f) x = std::tanh(x);
                else          x = std::atan(x * 1.2f) / 1.2f;
            }

            x = filter2.processSample(ch, x);

            if (drive2 > 1.0f)
            {
                x *= drive2;
                x = juce::jlimit(-0.95f, 0.95f, x);
            }

            channelData[sample] = x;
        }
    }

    auto* fxMixPtr = apvts.getRawParameterValue("fxMix");
    float fxMix = (fxMixPtr != nullptr) ? fxMixPtr->load() : 0.2f;
    fxMix = juce::jlimit(0.0f, 1.0f, fxMix);

    if (fxMix > 0.0f)
    {
        auto* delayTimePtr = apvts.getRawParameterValue("delayTime");
        float delaySecs = (delayTimePtr != nullptr) ? delayTimePtr->load() : 0.3f;
        
        if (currentSampleRate > 0)
            delayModule.setDelay(juce::jlimit(0.0f, 2.0f, delaySecs) * currentSampleRate);

        auto* feedbackPtr = apvts.getRawParameterValue("delayFeedback");
        float feedback = (feedbackPtr != nullptr) ? juce::jlimit(0.0f, 0.95f, feedbackPtr->load()) : 0.5f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < totalNumOutputChannels; ++ch)
            {
                float inputSample = buffer.getReadPointer(ch)[sample];
                float delayedSample = delayModule.popSample(ch);
                
                delayModule.pushSample(ch, inputSample + (delayedSample * feedback));
                buffer.getWritePointer(ch)[sample] += delayedSample * fxMix;
            }
        }

        auto* reverbSizePtr = apvts.getRawParameterValue("reverbSize");
        reverbParameters.roomSize = (reverbSizePtr != nullptr) ? juce::jlimit(0.0f, 1.0f, reverbSizePtr->load()) : 0.5f;
        reverbParameters.wetLevel = fxMix * 0.5f; 
        reverbParameters.dryLevel = 1.0f;
        reverbModule.setParameters(reverbParameters);
        
        juce::dsp::AudioBlock<float> mainBlock(buffer);
        juce::dsp::ProcessContextReplacing<float> mainContext(mainBlock);
        reverbModule.process(mainContext);
    }

    auto* masterGainPtr = apvts.getRawParameterValue("masterVolume");
    float masterGain = (masterGainPtr != nullptr) ? masterGainPtr->load() : 0.8f;
    buffer.applyGain(juce::jlimit(0.0f, 1.0f, masterGain));

    // Scrittura sicura dei campioni nell'oscilloscopio
    for (int i = 0; i < numSamples; ++i)
    {
        visualizerBuffer[visualizerWriteIndex] = buffer.getSample(0, i);
        visualizerWriteIndex = (visualizerWriteIndex + 1) % fftSize;
    }
    hasNewSourceData = true;
}

juce::AudioProcessorEditor* SambucaAudioProcessor::createEditor() { return new SambucaAudioProcessorEditor (*this); }
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
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SambucaAudioProcessor(); }
