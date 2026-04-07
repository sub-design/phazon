cmake_minimum_required(VERSION 3.22)
project(Phazon VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE via FetchContent
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Plugin target
# IS_SYNTH=TRUE → AU type is automatically kAudioUnitType_MusicDevice
# AU is macOS-only; Windows builds VST3 + Standalone only
# ---------------------------------------------------------------------------
if(APPLE)
    set(PHAZON_FORMATS VST3 AU Standalone)
else()
    set(PHAZON_FORMATS VST3 Standalone)
endif()


juce_add_plugin(Phazon
    VERSION                     0.1.0
    COMPANY_NAME                "Phazon"
    DESCRIPTION                 "MPE-aware nonlinear physical-modeling synthesizer"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Phzn
    PLUGIN_CODE                 Phz1
    FORMATS                     ${PHAZON_FORMATS}
    PRODUCT_NAME                "Phazon"
    VST3_CATEGORIES             "Instrument" "Synth"
)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
target_sources(Phazon
    PRIVATE
        src/PluginProcessor.h
        src/PluginProcessor.cpp
        src/PluginEditor.h
        src/PluginEditor.cpp
        src/engine/NetworkIf.h
        src/engine/NetworkIf.cpp
        src/synth/NetworkVoice.h
        src/synth/NetworkVoice.cpp
        src/synth/PhazonSynthesiser.h
        src/synth/PhazonSynthesiser.cpp
)

target_include_directories(Phazon
    PRIVATE
        src
        src/engine
        src/synth
)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(Phazon
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(Phazon
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
