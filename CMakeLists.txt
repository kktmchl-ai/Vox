cmake_minimum_required(VERSION 3.22)

set(VC_PRODUCT_NAME  "VoxChain")
set(VC_COMPANY_NAME  "DIY")
set(VC_MANUFACTURER_CODE "Diyz")
set(VC_PLUGIN_CODE       "Vcvx")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(VoxChain VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                              GIT_TAG 9.0.2 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(VoxChain
    COMPANY_NAME            "${VC_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${VC_MANUFACTURER_CODE}
    PLUGIN_CODE              ${VC_PLUGIN_CODE}
    FORMATS                  VST3
    PRODUCT_NAME             "${VC_PRODUCT_NAME}"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    VST3_CATEGORIES          Fx Channel_Strip
    COPY_PLUGIN_AFTER_BUILD  FALSE)

target_sources(VoxChain PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(VoxChain PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(VoxChain
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

if(MINGW)
    target_link_options(VoxChain PRIVATE -static -static-libgcc -static-libstdc++)
    target_compile_options(VoxChain PRIVATE -Wa,-mbig-obj)
endif()

option(VC_BUILD_TESTS "Build the headless wrapper test" OFF)
if(VC_BUILD_TESTS)
    juce_add_console_app(VoxChainWrapperTest PRODUCT_NAME "wrapper_test")
    target_sources(VoxChainWrapperTest PRIVATE tests/wrapper_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(VoxChainWrapperTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="VoxChain")
    target_link_libraries(VoxChainWrapperTest PRIVATE juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

    juce_add_console_app(VoxChainRenderTest PRODUCT_NAME "render_test")
    target_sources(VoxChainRenderTest PRIVATE tests/render_editor.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(VoxChainRenderTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="VoxChain")
    target_link_libraries(VoxChainRenderTest PRIVATE juce::juce_audio_utils juce::juce_gui_extra
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
