# img2sample - Image to Audio Sample Plugin
# Copyright (C) 2026
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)

project(Img2Sample VERSION 1.0.0 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dev mode flag (set to ON for hot-reload from localhost:8080) ---
option(IMG2SAMPLE_DEV "Enable dev mode (load UI from localhost)" OFF)

# --- Fetch JUCE ---
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# --- Plugin Target ---
juce_add_plugin(Img2Sample
    COMPANY_NAME            "img2sample"
    PLUGIN_MANUFACTURER_CODE I2sa
    PLUGIN_CODE              I2s1
    FORMATS                  VST3 Standalone AU
    PRODUCT_NAME             "img2sample"
    BUNDLE_ID                "com.img2sample.plugin"
    PLUGIN_NAME              "img2sample"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD  TRUE
    VST3_CATEGORIES          "Generator"
)

juce_generate_juce_header(Img2Sample)

# --- Source Files ---
target_sources(Img2Sample
    PRIVATE
        plugin/Source/PluginProcessor.cpp
        plugin/Source/PluginProcessor.h
        plugin/Source/PluginEditor.cpp
        plugin/Source/PluginEditor.h
        plugin/Source/Network/LyriaClient.cpp
        plugin/Source/Network/LyriaClient.h
        plugin/Source/Network/WebBridge.cpp
        plugin/Source/Network/WebBridge.h
        plugin/Source/UI/NativeDragView.h
        plugin/Source/UI/NativeDragView.mm
)

# --- Compile Definitions ---
target_compile_definitions(Img2Sample
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=0
)

# Dev mode preprocessor flag
if(IMG2SAMPLE_DEV)
    target_compile_definitions(Img2Sample PUBLIC DEV=1)
endif()

# --- Link Libraries ---
target_link_libraries(Img2Sample
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# --- Include Directories ---
target_include_directories(Img2Sample
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Source
)

# --- Copy Resources into app/plugin bundles ---
set(RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Resources/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Resources/bridge.js
)

# Copy resources into each target's build bundle
foreach(_target Img2Sample_Standalone Img2Sample_AU Img2Sample_VST3)
    if(TARGET ${_target})
        add_custom_command(TARGET ${_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${_target}>/../Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${RESOURCE_FILES}
                "$<TARGET_FILE_DIR:${_target}>/../Resources/"
            COMMENT "Copying web resources into ${_target} bundle"
        )
    endif()
endforeach()

# Also copy resources into the system-installed plugin bundles
# (JUCE's COPY_PLUGIN_AFTER_BUILD doesn't include Resources/)
if(APPLE)
    if(TARGET Img2Sample_VST3)
        add_custom_command(TARGET Img2Sample_VST3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/img2sample.vst3/Contents/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${RESOURCE_FILES}
                "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/img2sample.vst3/Contents/Resources/"
            COMMAND codesign --force --deep --sign -
                "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/img2sample.vst3"
            COMMENT "Copying web resources into installed VST3 bundle and re-signing"
        )
    endif()
    if(TARGET Img2Sample_AU)
        add_custom_command(TARGET Img2Sample_AU POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$ENV{HOME}/Library/Audio/Plug-Ins/Components/img2sample.component/Contents/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${RESOURCE_FILES}
                "$ENV{HOME}/Library/Audio/Plug-Ins/Components/img2sample.component/Contents/Resources/"
            COMMAND codesign --force --deep --sign -
                "$ENV{HOME}/Library/Audio/Plug-Ins/Components/img2sample.component"
            COMMENT "Copying web resources into installed AU bundle and re-signing"
        )
    endif()
endif()
