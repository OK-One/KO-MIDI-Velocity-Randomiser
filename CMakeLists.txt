cmake_minimum_required(VERSION 3.15.0)

project(VelocityRandomiser
    VERSION 1.0.0
    DESCRIPTION "MIDI velocity randomiser (VST 3)"
)

# Path to the VST 3 SDK folder (the one containing the SDK's root CMakeLists.txt), e.g.
#   cmake -G "Visual Studio 17 2022" -A x64 -Dvst3sdk_SOURCE_DIR=C:/SDKs/vst3sdk ..
set(vst3sdk_SOURCE_DIR "" CACHE PATH "Path to the VST 3 SDK")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Please set vst3sdk_SOURCE_DIR to your VST 3 SDK folder.")
endif()

# Build only this plug-in: skip Steinberg's example plug-ins and hosting examples, and VSTGUI
# (the editor is a small native Windows window, so VSTGUI is not needed).
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(VelocityRandomiser
    source/version.h
    source/vr_cids.h
    source/vr_velocity.h
    source/vr_midi.h
    source/vr_debug.h
    source/vr_editor.h
    source/vr_editor.cpp
    source/vr_processor.h
    source/vr_processor.cpp
    source/vr_controller.h
    source/vr_controller.cpp
    source/vr_entry.cpp
)

target_link_libraries(VelocityRandomiser
    PRIVATE
        sdk
)

smtg_target_configure_version_file(VelocityRandomiser)

if(SMTG_WIN)
    target_link_libraries(VelocityRandomiser PRIVATE comctl32)
endif()

if(SMTG_WIN)
    # Optional: a resource file gives the DLL its Windows "Details" properties. Generate a
    # project with the VST 3 Project Generator and copy its resource/win32resource.rc here.
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resource/win32resource.rc)
        target_sources(VelocityRandomiser PRIVATE resource/win32resource.rc)
    endif()
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VelocityRandomiser)
    endif()
endif()
