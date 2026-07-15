#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

struct SambucaOscillator 
{
    enum class Mode { StandardWave, MorphWave, LoadedSample };
    Mode currentMode = Mode::StandardWave;
    
    // Generatore a fase manuale per garantire morphing perfetto e stabilità
    double phase = 0.0;
    double phaseIncrement = 0.0;
    
    juce::AudioBuffer<float>* sampleBufferRef = nullptr;
    double samplePosition = 0.0;
    double sampleIncrement = 0.0;
    bool isLooping = true;
    int waveType = 0;

    void prepare (const juce::dsp::ProcessSpec& spec) 
    {
        phase = 0.0;
        samplePosition = 0.0;
    }
    
    void setFrequency (float frequency, double hostSampleRate)
    {
        if (hostSampleRate > 0)
        {
            phaseIncrement = frequency / hostSampleRate;
            
            if (currentMode == Mode::LoadedSample && sampleBufferRef != nullptr)
            {
                if (sampleBufferRef->getNumSamples() <= 100)
                {
                    sampleIncrement = 0.0;
                    return;
                }
                float rootFreq = juce::MidiMessage::getMidiNoteInHertz (60); 
                double pitchRatio = frequency / rootFreq;
                sampleIncrement = pitchRatio;
            }
        }
    }

    void setWaveform (int type)
    {
        waveType = type;
        // Convenzione: 0:Sine, 1:Saw, 2:Square, 3:Wavetable (Morph), 4:Sample, 5:Noise
        if (type == 3)
        {
            currentMode = Mode::MorphWave;
        }
        else if (type == 4)
        {
            currentMode = Mode::LoadedSample;
        }
        else
        {
            currentMode = Mode::StandardWave;
        }
    }

    float processSample (float morphValue)
    {
        // Avanzamento fase per sintesi oscillator
        phase += phaseIncrement;
        if (phase >= 1.0) phase -= 1.0;

        if (currentMode == Mode::StandardWave)
        {
            switch (waveType)
            {
                case 0: // Sine
                    return std::sin (phase * 2.0 * juce::MathConstants<double>::pi);
                case 1: // Saw
                    return static_cast<float>(2.0 * phase - 1.0);
                case 2: // Square
                    return phase < 0.5 ? -0.8f : 0.8f;
                case 5: // Noise
                    return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
                default:
                    return 0.0f;
            }
        }

        if (currentMode == Mode::MorphWave)
        {
            // Calcola le tre forme fondamentali sulla stessa fase per evitare cancellazioni
            float sine = std::sin (phase * 2.0 * juce::MathConstants<double>::pi);
            float saw = static_cast<float>(2.0 * phase - 1.0);
            float square = phase < 0.5 ? -0.8f : 0.8f;

            // Interpolazione lineare continua basata sul parametro Morphing (0.0 -> 1.0)
            if (morphValue < 0.5f)
            {
                float t = morphValue * 2.0f; // Riscalato [0, 1]
                return (1.0f - t) * sine + t * saw; // Morph da Sine a Saw
            }
            else
            {
                float t = (morphValue - 0.5f) * 2.0f; // Riscalato [0, 1]
                return (1.0f - t) * saw + t * square; // Morph da Saw a Square
            }
        }

        if (currentMode == Mode::LoadedSample && sampleBufferRef != nullptr && sampleBufferRef->getNumSamples() > 100)
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
            
            float s1 = sampleBufferRef->getSample (0, index1 % numSamples);
            float s2 = (index2 < numSamples || isLooping) ? sampleBufferRef->getSample (0, index2 % numSamples) : 0.0f;

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
        phase = 0.0;
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

        updateOscillators(0.0f);
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
            
        // Prepariamo tutti e 3 gli LFO di voce
        for (int i = 0; i < 3; ++i)
        {
            voiceLFOs[i].prepare (spec);
            voiceLFOs[i].initialise ([] (float x) { return std::sin (x); });
        }

        voiceFilter1.prepare (spec);
        
        adsr.setSampleRate (sampleRate);
        juce::ADSR::Parameters defaultParams;
        defaultParams.attack = 0.1f;
        defaultParams.decay = 0.1f;
        defaultParams.sustain = 1.0f;
        defaultParams.release = 0.2f;
        adsr.setParameters (defaultParams);
    }

    void updateOscillators (float pitchModulationSemitones)
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
                float detuneSemitones = pitchParam->load() + pitchModulationSemitones;
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

        // Configura la frequenza dei 3 LFO di questa voce
        for (int i = 0; i < 3; ++i)
        {
            juce::String prefix = "lfo" + juce::String(i + 1);
            auto* lfoRateParam = apvts.getRawParameterValue(prefix + "Rate");
            float lfoRate = (lfoRateParam != nullptr) ? lfoRateParam->load() : 1.0f;
            voiceLFOs[i].setFrequency (lfoRate);
        }

        // Aggiorna ADSR
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

        // Configurazione filtro di voce
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

        // Volumi individuali degli oscillatori
        float v1 = apvts.getRawParameterValue("osc1Volume") ? apvts.getRawParameterValue("osc1Volume")->load() : 0.7f;
        float v2 = apvts.getRawParameterValue("osc2Volume") ? apvts.getRawParameterValue("osc2Volume")->load() : 0.7f;
        float v3 = apvts.getRawParameterValue("osc3Volume") ? apvts.getRawParameterValue("osc3Volume")->load() : 0.7f;

        auto* morphParam = apvts.getRawParameterValue("wavetableMorph");
        float morphValue = (morphParam != nullptr) ? morphParam->load() : 0.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Calcoliamo le uscite correnti di tutti e 3 gli LFO
            float lfoOut[3];
            for (int i = 0; i < 3; ++i)
                lfoOut[i] = voiceLFOs[i].processSample (0.0f); // Range [-1.0, 1.0]

            // Inizializziamo le modulazioni cumulative
            float filterModulation = 0.0f;
            float volumeModulation = 1.0f;
            float pitchModulation = 0.0f;

            // Applichiamo e sommiamo le modulazioni da tutti e 3 gli LFO basandoci sul loro target
            for (int i = 0; i < 3; ++i)
            {
                juce::String prefix = "lfo" + juce::String(i + 1);
                auto* amountParam = apvts.getRawParameterValue(prefix + "Amount");
                auto* targetParam = apvts.getRawParameterValue(prefix + "Target");
                
                float lfoAmount = (amountParam != nullptr) ? amountParam->load() : 0.0f;
                int lfoTarget = (targetParam != nullptr) ? static_cast<int>(targetParam->load()) : 0; // 0=None, 1=Cutoff, 2=Volume, 3=Pitch

                float modulatedVal = lfoOut[i] * lfoAmount;

                if (lfoTarget == 1)      // Filter Cutoff
                    filterModulation += modulatedVal * 6000.0f; // Fino a 6kHz di escursione accumulata
                else if (lfoTarget == 2) // Volume
                    volumeModulation *= juce::jlimit(0.0f, 1.0f, 1.0f - (std::abs(modulatedVal) * 0.9f));
                else if (lfoTarget == 3) // Pitch (in semitoni)
                    pitchModulation += modulatedVal * 12.0f; // Fino a un'ottava
            }

            // Aggiorna frequenze oscillatori
            updateOscillators(pitchModulation);

            // Generazione e somma dei 3 oscillatori indipendenti
            float s1 = oscillators[0].processSample (morphValue) * v1;
            float s2 = oscillators[1].processSample (morphValue) * v2;
            float s3 = oscillators[2].processSample (morphValue) * v3;

            // Somma reale
            float sampleSum = s1 + s2 + s3;

            // Applica ADSR, Velocity e modulazione del volume
            float envelope = adsr.getNextSample();
            float voiceSample = sampleSum * envelope * noteVelocity * volumeModulation;

            // Calcolo del cutoff modulato
            float modulatedCutoff1 = juce::jlimit (20.0f, 20000.0f, baseCutoff1 + filterModulation);
            voiceFilter1.setCutoffFrequencyHz (modulatedCutoff1);

            // CORREZIONE: Somma il campione a quello già esistente nel buffer usando addSample invece di setSample
            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            {
                outputBuffer.addSample (channel, startSample + sample, voiceSample);
            }

            juce::dsp::AudioBlock<float> singleSampleBlock (outputBuffer);
            juce::dsp::AudioBlock<float> subBlock = singleSampleBlock.getSubBlock (startSample + sample, 1);
            juce::dsp::ProcessContextReplacing<float> context (subBlock);
            voiceFilter1.process (context);
        }

        if (!adsr.isActive()) clearCurrentNote();
    }

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
    
    // Array di 3 LFO reali e attivi
    juce::dsp::Oscillator<float> voiceLFOs[3];
    juce::dsp::LadderFilter<float> voiceFilter1;
};
