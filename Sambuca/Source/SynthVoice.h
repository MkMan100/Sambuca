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
        if (currentMode == Mode::StandardWave) waveOsc.setFrequency (frequency);
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
    // Passiamo direttamente l'APVTS nativo di JUCE, così non serve sapere cos'è il SambucaAudioProcessor!
    SynthVoice (juce::AudioProcessorValueTreeState& vts) : apvts (vts)
    {
        for (int i = 0; i < 3; ++i)
            oscillators[i].waveOsc.initialise ([] (float x) { return std::sin (x); });
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override { return dynamic_cast<SynthSound*>(sound) != nullptr; }
    
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
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
        // FISSA IL CRASH DELL'LFO: Fornisci una funzione generatrice iniziale (Sine)
        voiceLFO.initialise ([] (float x) { return std::sin (x); });

        voiceFilter1.prepare (spec);
        
        // FISSA IL CRASH DELL'ADSR: Imposta i campioni al secondo e parametri di default
        adsr.setSampleRate (sampleRate);
        juce::ADSR::Parameters defaultParams;
        defaultParams.attack = 0.1f;
        defaultParams.decay = 0.1f;
        defaultParams.sustain = 1.0f;
        defaultParams.release = 0.2f;
        adsr.setParameters (defaultParams);
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (!isVoiceActive()) return;

        // Recupero sicuro con puntatori di controllo (Evita crash se l'APVTS non è ancora pronto)
        auto* morphParam = apvts.getRawParameterValue("wavetableMorph");
        auto* lfoRateParam = apvts.getRawParameterValue("lfo1Rate");
        auto* lfoAmountParam = apvts.getRawParameterValue("lfo1Amount");
        
        float morphValue = (morphParam != nullptr) ? morphParam->load() : 0.0f;
        float lfoRate = (lfoRateParam != nullptr) ? lfoRateParam->load() : 1.0f;
        float lfoAmount = (lfoAmountParam != nullptr) ? lfoAmountParam->load() : 0.0f;

        voiceLFO.setFrequency (lfoRate);

        // --- AGGIORNAMENTO DINAMICO ADSR DA INTERFACCIA ---
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

        auto* cutoffParam = apvts.getRawParameterValue("filter1Cutoff");
        auto* resParam = apvts.getRawParameterValue("filter1Resonance");
        
        float baseCutoff1 = (cutoffParam != nullptr) ? cutoffParam->load() : 1000.0f;
        float res1 = (resParam != nullptr) ? resParam->load() : 1.0f;
        
        float modulatedCutoff1 = juce::jlimit (20.0f, 20000.0f, baseCutoff1 + (lastLfoValue * 5000.0f));
        
        voiceFilter1.setCutoffFrequencyHz (modulatedCutoff1);
        voiceFilter1.setResonance (res1);

        juce::dsp::AudioBlock<float> block (synthBlock);
        juce::dsp::ProcessContextReplacing<float> context (block);
        voiceFilter1.process (context);

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            outputBuffer.addFrom (channel, startSample, synthBlock.getReadPointer(channel), numSamples);

        if (!adsr.isActive()) clearCurrentNote();
    }
private:
    juce::AudioProcessorValueTreeState& apvts; // Collegamento diretto e pulito
    SambucaOscillator oscillators[3];
    juce::ADSR adsr;
    float noteVelocity = 0.0f;
    juce::dsp::Oscillator<float> voiceLFO;
    juce::dsp::LadderFilter<float> voiceFilter1;
};
