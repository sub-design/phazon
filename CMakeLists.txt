cmake_minimum_required(VERSION 3.22)
project(Phazon VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE dependency
# Prefer an explicitly supplied local JUCE source or config for offline builds:
#   cmake -S . -B build -DJUCE_SOURCE_DIR=/path/to/JUCE
#   cmake -S . -B build -DJUCE_DIR=/path/to/JUCE
# Fallback to FetchContent when no local package is provided.
# ---------------------------------------------------------------------------
if (DEFINED JUCE_SOURCE_DIR)
    add_subdirectory (${JUCE_SOURCE_DIR} JUCE)
elseif (DEFINED JUCE_DIR)
    find_package(JUCE CONFIG REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

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
        src/ParameterIDs.h
        src/PhazonReverb.h
        src/PostProcessChain.h
        src/engine/NetworkIf.h
        src/engine/NetworkIf.cpp
        src/synth/NetworkVoice.h
        src/synth/NetworkVoice.cpp
        src/synth/PhazonSynthesiser.h
        src/synth/PhazonSynthesiser.cpp
        src/presets/PresetManager.h
        src/presets/PresetManager.cpp
        src/ui/PhazonLookAndFeel.h
        src/ui/PresetBrowser.h
        src/ui/StringVisualizer.h
)

target_include_directories(Phazon
    PRIVATE
        src
        src/engine
        src/synth
        src/presets
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

add_executable(phazon_verify
    tools/phazon_verify.cpp
    src/engine/NetworkIf.cpp
    src/synth/NetworkVoice.cpp
    src/synth/PhazonSynthesiser.cpp
)

target_include_directories(phazon_verify
    PRIVATE
        src
        src/engine
        src/synth
)

target_link_libraries(phazon_verify
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(phazon_verify
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)
