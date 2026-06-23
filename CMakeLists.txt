cmake_minimum_required(VERSION 3.15)

project(Sambuca VERSION 1.0.0 LANGUAGES CXX)

# Include il modulo FetchContent di CMake per scaricare JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.5 # Versione stabile e collaudata
)
FetchContent_MakeAvailable(JUCE)

# Crea il formato VST3 e la versione Standalone del Synth
juce_add_plugin(Sambuca
    COMPANY_NAME "MkMan"
    PLUGIN_MANUFACTURER_CODE "MkMn"
    PLUGIN_CODE "Sabc"
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
)

# Elenco dei file sorgente
target_sources(Sambuca PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SynthVoice.h
    Source/SynthSound.h
)

# Abilita i moduli JUCE fondamentali (senza SambucaAssets)
target_link_libraries(Sambuca PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
)

# Forza lo standard C++17 per garantire la compatibilità sul server GitHub
target_compile_features(Sambuca PRIVATE cxx_std_17)
