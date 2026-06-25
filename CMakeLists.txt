cmake_minimum_required(VERSION 3.15)

project(Sambuca VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.5
)
FetchContent_MakeAvailable(JUCE)

# 1. DICHIARA I SEI FILE PNG COME BINARY DATA
juce_add_binary_data(SambucaAssets
    SOURCES
        Sambuca/Source/Assets/knob_base.png
        Sambuca/Source/Assets/knob_glow.png
        Sambuca/Source/Assets/background.png
)

juce_add_plugin(Sambuca
    COMPANY_NAME "MkMan"
    PLUGIN_MANUFACTURER_CODE "MkMn"
    PLUGIN_CODE "Sabc"
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
)

target_sources(Sambuca PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sambuca/Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sambuca/Source/PluginProcessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Sambuca/Source/PluginEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sambuca/Source/PluginEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Sambuca/Source/SynthVoice.h
)

# 2. COLLEGA I BINARY DATA AL PLUGIN
target_link_libraries(Sambuca PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    SambucaAssets # <--- Collegamento qui
)

target_compile_features(Sambuca PRIVATE cxx_std_17)
target_compile_definitions(Sambuca PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)
