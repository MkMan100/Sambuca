#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

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
        if (currentMode == Mode::StandardWave) 
        {
            waveOsc.setFrequency (frequency);
        }
        else if (currentMode == Mode::LoadedSample && sampleBufferRef != nullptr && hostSampleRate > 0)
        {
            // Calcola la velocità di riproduzione del sample basata sulla nota suonata (assumendo il sample originale a C4 = 60)
            // Se preferisci una riproduzione a velocità fissa stile "one-shot trigger", si può impostare a 1.0
            float rootFreq = juce::MidiMessage::getMidiNoteInHertz (60); 
            double pitchRatio = frequency / rootFreq;
            sampleIncrement = pitchRatio;
        }
    }

    void setWaveform (int type)
    {
        // Convenzione parametri: 0:Sine, 1:Saw, 2:Square, 3:Wavetable, 4:Sample, 5:Noise
        if (type == 3 || type == 4)
        {
            currentMode = Mode::LoadedSample;
        }
        else
        {
            currentMode = Mode::StandardWave;
            switch (type)
            {
                case 0: waveOsc.initialise ([] (float x) { return std::sin (x); }); break; // Sine
                case 1: waveOsc.initialise ([] (float x) { return x / juce::MathConstants<float>::pi; }); break; // Saw
                case 2: waveOsc.initialise ([] (float x) { return x < 0.0f ? -1.0f : 1.0f; }); break; // Square
                case 5: waveOsc.initialise ([] (float x) { return ((float)rand() / RAND_MAX) * 2.0f - 1.0f; }); break; // Noise
                default: waveOsc.initialise ([] (float x) { return std::sin (x); }); break;
            }
        }
    }

    float processSample()
    {
        if (currentMode == Mode::StandardWave)
            return waveOsc.processSample (0.0f);

        if (currentMode == Mode::LoadedSample && sampleBufferRef != nullptr && sampleBufferRef->getNumSamples() > 0)
        {
            auto numSamples = sampleBufferRef->getNumSamples();
            int index1 = static_cast<int> (samplePosition);
            int index2 = index1 + 1;

            if (isLooping)
            {
                index1 %= numSamples;
                index2 %= numSamples;
            }
            else if (index1 >= numSamples)
            {
                return 0.0f;
            }

            float fraction = static_cast<float> (samplePosition - static_cast<int> (samplePosition));
            
            // Lettura mono (canale 0) del sample caricato
            float s1 = sampleBufferRef->getSample (0, index1);
            float s2 = (index2 < numSamples || isLooping) ? sampleBufferRef->getSample (0, index2) : 0.0f;

            // Interpolazione lineare
            float output = s1 + fraction * (s2 - s1);

            samplePosition += sampleIncrement;
            if (isLooping && samplePosition >= numSamples)
                samplePosition = std::fmod (samplePosition, static_cast<double> (numSamples));

            return output;
        }

        return 0.0f;
    }

    void resetSamplePosition()
    {
        samplePosition = 0.0;
    }
};

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice (juce::AudioProcessorValueTreeState& vts) : apvts (vts)
    {
        for (int i = 0; i < 3; ++i)
            oscillators[i].setWaveform(0);
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<SynthSound*>(sound) != nullptr; }
    
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        noteVelocity = velocity;
        currentMidiNote = midiNoteNumber;
        
        for (int i = 0; i < 3; ++i)
            oscillators[i].resetSamplePosition();

        updateOscillators();
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        adsr.noteOff();
        if (!allowTailOff || !adsr.isActive()) clearCurrentNote();
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels)
    {
        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), static_cast<juce::uint32> (outputChannels) };
        
        for (int i = 0; i < 3; ++i) 
            oscillators[i].prepare (spec);
            
        voiceLFO.prepare (spec);
        voiceLFO.initialise ([] (float x) { return std::sin (x); });

        voiceFilter1.prepare (spec);
        
        adsr.setSampleRate (sampleRate);
        juce::ADSR::Parameters defaultParams;
        defaultParams.attack = 0.1f;
        defaultParams.decay = 0.1f;
        defaultParams.sustain = 1.0f;
        defaultParams.release = 0.2f;
        adsr.setParameters (defaultParams);
    }

    void updateOscillators()
    {
        float baseFreq = juce::MidiMessage::getMidiNoteInHertz (currentMidiNote);

        for (int i = 0; i < 3; ++i)
        {
            juce::String prefix = "osc" + juce::String(i + 1);
            
            auto* waveParam = apvts.getRawParameterValue(prefix + "Waveform");
            auto* pitchParam = apvts.getRawParameterValue(prefix + "Pitch");

            if (waveParam != nullptr)
                oscillators[i].setWaveform (static_cast<int>(waveParam->load()));

            if (pitchParam != nullptr)
            {
                float detuneSemitones = pitchParam->load();
                float finalFreq = baseFreq * std::pow (2.0f, detuneSemitones / 12.0f);
                oscillators[i].setFrequency (finalFreq, getSampleRate());
            }
            else
            {
                oscillators[i].setFrequency (baseFreq, getSampleRate());
            }
        }
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (!isVoiceActive()) return;

        updateOscillators();

        auto* morphParam = apvts.getRawParameterValue("wavetableMorph");
        auto* lfoRateParam = apvts.getRawParameterValue("lfo1Rate");
        auto* lfoAmountParam = apvts.getRawParameterValue("lfo1Amount");
        
        float morphValue = (morphParam != nullptr) ? morphParam->load() : 0.0f;
        float lfoRate = (lfoRateParam != nullptr) ? lfoRateParam->load() : 1.0f;
        float lfoAmount = (lfoAmountParam != nullptr) ? lfoAmountParam->load() : 0.0f;

        voiceLFO.setFrequency (lfoRate);

        auto* att = apvts.getRawParameterValue("attack");
        auto* dec = apvts.getRawParameterValue("decay");
        auto* sus = apvts.getRawParameterValue("sustain");
        auto* rel = apvts.getRawParameterValue("release");

        if (att != nullptr && dec != nullptr && sus != nullptr && rel != nullptr)
        {
            juce::ADSR::Parameters p;
            p.attack  = att->load();
            p.decay   = dec->load();
            p.sustain = sus->load();
            p.release = rel->load();
            adsr.setParameters (p);
        }

        auto* typeParam = apvts.getRawParameterValue("filter1Type");
        auto* resParam = apvts.getRawParameterValue("filter1Resonance");
        auto* cutoffParam = apvts.getRawParameterValue("filter1Cutoff");
        
        int filterType = (typeParam != nullptr) ? static_cast<int>(typeParam->load()) : 0;
        float res1 = (resParam != nullptr) ? resParam->load() : 1.0f;
        float baseCutoff1 = (cutoffParam != nullptr) ? cutoffParam->load() : 1000.0f;

        using FilterMode = typename juce::dsp::LadderFilter<float>::Mode;
        switch (filterType)
        {
            case 0: voiceFilter1.setMode (static_cast<FilterMode> (0)); break; 
            case 1: voiceFilter1.setMode (static_cast<FilterMode> (1)); break; 
            case 2: voiceFilter1.setMode (static_cast<FilterMode> (2)); break; 
            case 3: voiceFilter1.setMode (static_cast<FilterMode> (2)); break; 
        }
        voiceFilter1.setResonance (res1);

        juce::AudioBuffer<float> synthBlock (outputBuffer.getNumChannels(), numSamples);
        synthBlock.clear();

        float v1 = apvts.getRawParameterValue("osc1Volume") ? apvts.getRawParameterValue("osc1Volume")->load() : 0.7f;
        float v2 = apvts.getRawParameterValue("osc2Volume") ? apvts.getRawParameterValue("osc2Volume")->load() : 0.7f;
        float v3 = apvts.getRawParameterValue("osc3Volume") ? apvts.getRawParameterValue("osc3Volume")->load() : 0.7f;

        juce::AudioBuffer<float> singleSampleBuffer (synthBlock.getNumChannels(), 1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float currentLfoValue = voiceLFO.processSample (0.0f) * lfoAmount;
            
            // Utilizzo del nuovo metodo unificato processSample() che discrimina internamente la modalità
            float s1 = oscillators[0].processSample();
            float s2 = oscillators[1].processSample();
            float s3 = oscillators[2].processSample();

            s1 *= v1; s2 *= v2; s3 *= v3;

            float sampleSum = 0.0f;
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
            
            for (int channel = 0; channel < singleSampleBuffer.getNumChannels(); ++channel)
                singleSampleBuffer.setSample (channel, 0, voiceSample);

            float modulatedCutoff1 = juce::jlimit (20.0f, 20000.0f, baseCutoff1 + (currentLfoValue * 5000.0f));
            voiceFilter1.setCutoffFrequencyHz (modulatedCutoff1);

            juce::dsp::AudioBlock<float> singleBlock (singleSampleBuffer);
            juce::dsp::ProcessContextReplacing<float> context (singleBlock);
            voiceFilter1.process (context);

            for (int channel = 0; channel < synthBlock.getNumChannels(); ++channel)
                synthBlock.setSample (channel, sample, singleSampleBuffer.getSample (channel, 0));
        }

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            outputBuffer.addFrom (channel, startSample, synthBlock.getReadPointer(channel), numSamples);

        if (!adsr.isActive()) clearCurrentNote();
    }

    // Funzione pubblica per agganciare i buffer allocati nel Processor a ciascuna voce
    void setSampleBufferPointer (int index, juce::AudioBuffer<float>* buffer)
    {
        if (index >= 0 && index < 3)
            oscillators[index].sampleBufferRef = buffer;
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    SambucaOscillator oscillators[3];
    juce::ADSR adsr;
    float noteVelocity = 0.0f;
    int currentMidiNote = 60;
    juce::dsp::Oscillator<float> voiceLFO;
    juce::dsp::LadderFilter<float> voiceFilter1;
};
