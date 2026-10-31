cmake_minimum_required(VERSION 3.22)

# Runtime de Visual C++ enlazado de forma estatica: el plugin NO necesita instalar nada extra en Windows.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(HardVox VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE se descarga automaticamente de GitHub durante la compilacion (version fija 7.0.12)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# La imagen de la interfaz se incrusta dentro del plugin (no depende de archivos externos)
juce_add_binary_data(HardVoxAssets SOURCES Assets/background.png)
set_target_properties(HardVoxAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(HardVox
    COMPANY_NAME "HardVox"
    PLUGIN_MANUFACTURER_CODE Hvox
    PLUGIN_CODE Hvx1
    FORMATS VST3
    PRODUCT_NAME "HARD VOX"
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(HardVox PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Dsp.h
    Source/Params.h
    Source/Common.h)

target_compile_definitions(HardVox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(HardVox
    PRIVATE
        HardVoxAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags)
