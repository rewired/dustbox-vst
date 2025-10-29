cmake_minimum_required(VERSION 3.22)

project(Dustbox VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DUSTBOX_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(DUSTBOX_STRICT_BUILD "Treat warnings as errors" OFF)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/JUCE/CMakeLists.txt")
    message(STATUS "JUCE submodule not found. Please run 'git submodule update --init --recursive'.")
endif()

add_subdirectory(extern/JUCE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ToolchainWarnings)

juce_add_plugin(Dustbox
    COMPANY_NAME "7OOP3D"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS VST3
    PRODUCT_NAME "Dustbox")

target_sources(Dustbox PRIVATE
    Source/Plugin/DustboxProcessor.cpp
    Source/Plugin/DustboxEditor.cpp
    Source/Dsp/modules/TapeModule.cpp
    Source/Dsp/modules/DirtModule.cpp
    Source/Dsp/modules/PumpModule.cpp
    Source/Ui/GenericControls.cpp)

target_compile_features(Dustbox PRIVATE cxx_std_17)

target_include_directories(Dustbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_link_libraries(Dustbox
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra)

juce_generate_juce_header(Dustbox)

target_compile_definitions(Dustbox
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JucePlugin_ManufacturerCode=0x434D4C42 # CMLB
        JucePlugin_PluginCode=0x44627831 # Dbx1
        JucePlugin_Manufacturer="Communit Labs"
        JucePlugin_Name="Dustbox")

if(DUSTBOX_ENABLE_WARNINGS)
    dustbox_enable_warnings(Dustbox ${DUSTBOX_STRICT_BUILD})
endif()

set(dustbox_vst3_output_dir "${CMAKE_BINARY_DIR}/out")

add_custom_command(TARGET Dustbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${dustbox_vst3_output_dir}/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:Dustbox>/../.." "${dustbox_vst3_output_dir}/$<CONFIG>/Dustbox.vst3"
    COMMENT "Copying Dustbox.vst3 to ${dustbox_vst3_output_dir}/$<CONFIG>")

message(STATUS "Dustbox VST3 binaries will be available under ${CMAKE_BINARY_DIR}/VST3/<Config>/Dustbox.vst3")
message(STATUS "A copy is also placed in ${dustbox_vst3_output_dir}/<Config>/Dustbox.vst3 after build.")
