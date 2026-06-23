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

# ==============================================================================
# Inclusione Asset Grafici Sambuca
# ==============================================================================
juce_add_binary_data(SambucaAssets
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/background.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/knob_base.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/knob_glow.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/knob_waveform_6steps.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/slider_handle.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/button_load_normal.png
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/button_load_pressed.png
)
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

# Elenco dei file sorgente (che andremo a creare)
target_sources(Sambuca PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SynthVoice.h
    Source/SynthSound.h
)

# Abilita i moduli JUCE fondamentali (incluso il modulo DSP per filtri ed effetti)
target_link_libraries(Sambuca PRIVATE
    juce::juce_audio_utils
    SambucaAssets
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
)

# Forza lo standard C++17 per garantire la compatibilità sul server GitHub
target_compile_features(Sambuca PRIVATE cxx_std_17)
