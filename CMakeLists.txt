cmake_minimum_required(VERSION 3.22)

project(Dustbox VERSION 0.0.1 LANGUAGES CXX)

set(TARGET_NAME Dustbox)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DUSTBOX_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(DUSTBOX_STRICT_BUILD "Treat warnings as errors" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/JUCE/CMakeLists.txt")
    message(STATUS "JUCE submodule not found. Please run 'git submodule update --init --recursive'.")
else()
    include(JuceWarningPatches)
endif()

add_subdirectory(extern/JUCE)

include(ToolchainWarnings)

juce_add_plugin(${TARGET_NAME}
    COMPANY_NAME "7OOP3D"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD OFF
    FORMATS VST3
    PRODUCT_NAME "Dustbox")

target_sources(${TARGET_NAME} PRIVATE
    Source/Plugin/DustboxProcessor.cpp
    Source/Plugin/DustboxEditor.cpp
    Source/Presets/FactoryPresets.cpp
    Source/Dsp/modules/NoiseModule.cpp
    Source/Dsp/modules/TapeModule.cpp
    Source/Dsp/modules/DirtModule.cpp
    Source/Dsp/modules/PumpModule.cpp
    Source/Ui/GenericControls.cpp)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra)

juce_generate_juce_header(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JucePlugin_ManufacturerCode=0x434D4C42 # CMLB
        JucePlugin_PluginCode=0x44627831 # Dbx1
        JucePlugin_Manufacturer="Communit Labs"
        JucePlugin_Name="Dustbox")

if(DUSTBOX_ENABLE_WARNINGS)
    dustbox_enable_warnings(${TARGET_NAME} ${DUSTBOX_STRICT_BUILD})
endif()

# --- Deploy & install configuration ---------------------------------------------------------

option(DUSTBOX_ENABLE_POST_BUILD_DEPLOY "Copy the built VST3 bundle into the deploy directory after each build" OFF)

set(DUSTBOX_DEFAULT_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/_deploy")

function(dustbox_normalize_path out_path in_path)
    if("${in_path}" STREQUAL "")
        set(${out_path} "" PARENT_SCOPE)
        return()
    endif()

    if(EXISTS "${in_path}")
        file(REAL_PATH "${in_path}" _rp)
    else()
        get_filename_component(_rp "${in_path}" ABSOLUTE)
    endif()

    cmake_path(CONVERT "${_rp}" TO_CMAKE_PATH_LIST _rp)

    if(WIN32)
        string(TOLOWER "${_rp}" _rp)
    endif()

    set(${out_path} "${_rp}" PARENT_SCOPE)
endfunction()

function(dustbox_path_is_within result child_path parent_path)
    dustbox_normalize_path(_child "${child_path}")
    dustbox_normalize_path(_parent "${parent_path}")

    set(_is_within FALSE)

    if(NOT "${_child}" STREQUAL "" AND NOT "${_parent}" STREQUAL "")
        if(_child STREQUAL "${_parent}")
            set(_is_within TRUE)
        else()
            string(LENGTH "${_parent}" _parent_len)
            string(LENGTH "${_child}" _child_len)

            if(_child_len GREATER _parent_len)
                string(SUBSTRING "${_child}" 0 ${_parent_len} _child_prefix)
                if(_child_prefix STREQUAL "${_parent}")
                    string(SUBSTRING "${_child}" ${_parent_len} 1 _next_char)
                    if(_next_char STREQUAL "/" OR _next_char STREQUAL "\\")
                        set(_is_within TRUE)
                    endif()
                endif()
            endif()
        endif()
    endif()

    set(${result} ${_is_within} PARENT_SCOPE)
endfunction()

function(dustbox_assert_not_subpath child_path parent_path)
    dustbox_path_is_within(_is_within "${child_path}" "${parent_path}")

    if(_is_within)
        message(FATAL_ERROR
            "Unsafe deploy detected: destination '${child_path}' resides within source '${parent_path}'.")
    endif()
endfunction()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${DUSTBOX_DEFAULT_INSTALL_PREFIX}" CACHE PATH "Install destination prefix" FORCE)
endif()

# Ensure installations never land inside the active build tree.
dustbox_path_is_within(_install_inside_binary "${CMAKE_INSTALL_PREFIX}" "${CMAKE_BINARY_DIR}")
if(_install_inside_binary)
    message(WARNING
        "Install prefix '${CMAKE_INSTALL_PREFIX}' resolved inside the build tree. Overriding to '${DUSTBOX_DEFAULT_INSTALL_PREFIX}'.")
    set(CMAKE_INSTALL_PREFIX "${DUSTBOX_DEFAULT_INSTALL_PREFIX}" CACHE PATH "Install destination prefix" FORCE)
endif()

set(DUSTBOX_VST3_BUNDLE_NAME "${TARGET_NAME}.vst3")
set(DUSTBOX_VST3_BUNDLE_SRC "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>")
set(DUSTBOX_DEPLOY_DIR "${CMAKE_INSTALL_PREFIX}/VST3/$<CONFIG>")
set(DUSTBOX_VST3_DEPLOY_DST "${DUSTBOX_DEPLOY_DIR}/${DUSTBOX_VST3_BUNDLE_NAME}")

message(STATUS "Dustbox install prefix : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Dustbox VST3 bundle src: ${DUSTBOX_VST3_BUNDLE_SRC}")
message(STATUS "Dustbox deploy destination: ${DUSTBOX_VST3_DEPLOY_DST}")

if(NOT CMAKE_CONFIGURATION_TYPES)
    set(_dustbox_preflight_dst "${DUSTBOX_VST3_DEPLOY_DST}")
    if(CMAKE_BUILD_TYPE)
        string(REPLACE "$<CONFIG>" "${CMAKE_BUILD_TYPE}" _dustbox_preflight_dst "${_dustbox_preflight_dst}")
    endif()

    string(FIND "${DUSTBOX_VST3_BUNDLE_SRC}" "$<" _dustbox_has_gen)
    if(_dustbox_has_gen EQUAL -1)
        dustbox_assert_not_subpath("${_dustbox_preflight_dst}" "${DUSTBOX_VST3_BUNDLE_SRC}")
    endif()
endif()

if(DUSTBOX_ENABLE_POST_BUILD_DEPLOY)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cmake")
    set(_dustbox_copy_script "${CMAKE_CURRENT_BINARY_DIR}/cmake/dustbox_copy_vst3.cmake")
    set(_dustbox_copy_script_template [=[
cmake_minimum_required(VERSION 3.20)

function(_dustbox_normalize out_path in_path)
    if("${in_path}" STREQUAL "")
        set(${out_path} "" PARENT_SCOPE)
        return()
    endif()

    cmake_path(CONVERT "${in_path}" TO_CMAKE_PATH_LIST _converted)
    if(EXISTS "${_converted}")
        file(REAL_PATH "${_converted}" _rp)
    else()
        get_filename_component(_rp "${_converted}" ABSOLUTE)
    endif()
    cmake_path(CONVERT "${_rp}" TO_CMAKE_PATH_LIST _rp)
    if(WIN32)
        string(TOLOWER "${_rp}" _rp)
    endif()
    set(${out_path} "${_rp}" PARENT_SCOPE)
endfunction()

set(_dustbox_src "${src}")
set(_dustbox_dst "${dst}")

if(_dustbox_src STREQUAL "" OR _dustbox_dst STREQUAL "")
    message(FATAL_ERROR "[Deploy] Source or destination path not provided.")
endif()

_dustbox_normalize(_norm_src "${_dustbox_src}")
_dustbox_normalize(_norm_dst "${_dustbox_dst}")

string(LENGTH "${_norm_src}" _src_len)
string(LENGTH "${_norm_dst}" _dst_len)

if(_src_len GREATER 0 AND _dst_len GREATER_EQUAL _src_len)
    string(SUBSTRING "${_norm_dst}" 0 ${_src_len} _dst_prefix)
    if(_dst_prefix STREQUAL "${_norm_src}")
        if(_dst_len EQUAL _src_len)
            message(FATAL_ERROR "Unsafe deploy detected at build-time: destination '${_dustbox_dst}' matches source '${_dustbox_src}'.")
        else()
            string(SUBSTRING "${_norm_dst}" ${_src_len} 1 _next_char)
            if(_next_char STREQUAL "/" OR _next_char STREQUAL "\\")
                message(FATAL_ERROR "Unsafe deploy detected at build-time: destination '${_dustbox_dst}' lies within source '${_dustbox_src}'.")
            endif()
        endif()
    endif()
endif()

if(NOT EXISTS "${_dustbox_src}")
    message(FATAL_ERROR "[Deploy] VST3 bundle not found at '${_dustbox_src}'. Has the target been built?")
endif()

get_filename_component(_dst_parent "${_dustbox_dst}" DIRECTORY)
if(NOT _dst_parent STREQUAL "")
    file(MAKE_DIRECTORY "${_dst_parent}")
endif()

if(EXISTS "${_dustbox_dst}")
    file(REMOVE_RECURSE "${_dustbox_dst}")
endif()

message(STATUS "[Deploy] Copying VST3 from '${_dustbox_src}' to '${_dustbox_dst}'")
execute_process(COMMAND "@CMAKE_COMMAND@" -E copy_directory "${_dustbox_src}" "${_dustbox_dst}" RESULT_VARIABLE _copy_result)
if(NOT _copy_result EQUAL 0)
    message(FATAL_ERROR "[Deploy] Copy failed with code ${_copy_result}")
endif()
]=])
    string(CONFIGURE "${_dustbox_copy_script_template}" _dustbox_copy_script_configured @ONLY)
    file(WRITE "${_dustbox_copy_script}" "${_dustbox_copy_script_configured}")

    if(CMAKE_GENERATOR_PLATFORM)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E env
                "CMAKE_GENERATOR=${CMAKE_GENERATOR}"
                "CMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                "${CMAKE_COMMAND}" -Dsrc="$<TARGET_BUNDLE_DIR:${TARGET_NAME}>" -Ddst="${DUSTBOX_VST3_DEPLOY_DST}" -P "${_dustbox_copy_script}"
            VERBATIM
            COMMENT "Deploying ${DUSTBOX_VST3_BUNDLE_NAME} to ${DUSTBOX_DEPLOY_DIR}")
    else()
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E env
                "CMAKE_GENERATOR=${CMAKE_GENERATOR}"
                "${CMAKE_COMMAND}" -Dsrc="$<TARGET_BUNDLE_DIR:${TARGET_NAME}>" -Ddst="${DUSTBOX_VST3_DEPLOY_DST}" -P "${_dustbox_copy_script}"
            VERBATIM
            COMMENT "Deploying ${DUSTBOX_VST3_BUNDLE_NAME} to ${DUSTBOX_DEPLOY_DIR}")
    endif()
    message(STATUS "Post-build deploy enabled. Safe copy target: ${DUSTBOX_VST3_DEPLOY_DST}")
else()
    message(STATUS "Post-build deploy disabled. Use 'cmake --build <build-dir> --config <cfg> --target INSTALL' to deploy.")
endif()

install(TARGETS ${TARGET_NAME}
    BUNDLE  DESTINATION "VST3/$<CONFIG>"
    RUNTIME DESTINATION "VST3/$<CONFIG>")

message(STATUS "Install command: cmake --build <build-dir> --config Debug|Release --target INSTALL")
