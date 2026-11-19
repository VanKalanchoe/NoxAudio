cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendors/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vendors/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

project(NoxAudio VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_VS_GLOBALS "UseMultiToolTask=true" "EnforceProcessCountAcrossBuilds=true")
endif()
if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/MP>)
endif()

option(NOXAUDIO_BUILD_TESTS "Build standalone NoxAudio verification tests" ON)

# -----------------------------------------------------------------------------
# Third-Party Dependencies
# -----------------------------------------------------------------------------
find_package(glm CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

# miniaudio (header + stb_vorbis in extras/)
set(NOXAUDIO_MINIAUDIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendors/miniaudio" CACHE PATH "Path to miniaudio")
if(NOT EXISTS "${NOXAUDIO_MINIAUDIO_DIR}/miniaudio.h")
    message(FATAL_ERROR "miniaudio not found in '${NOXAUDIO_MINIAUDIO_DIR}'. Run scripts/install_dependencies.bat first.")
endif()

# Steam Audio SDK (phonon.h + phonon.lib / phonon.dll)
set(NOXAUDIO_STEAMAUDIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendors/steamaudio" CACHE PATH "Path to Steam Audio SDK")
if(NOT EXISTS "${NOXAUDIO_STEAMAUDIO_DIR}/include/phonon.h")
    message(FATAL_ERROR "Steam Audio SDK not found in '${NOXAUDIO_STEAMAUDIO_DIR}'. Run scripts/install_dependencies.bat first.")
endif()

if(WIN32)
    set(NOXAUDIO_PHONON_LIB "${NOXAUDIO_STEAMAUDIO_DIR}/lib/windows-x64/phonon.lib")
    set(NOXAUDIO_PHONON_DLL "${NOXAUDIO_STEAMAUDIO_DIR}/lib/windows-x64/phonon.dll")
elseif(APPLE)
    set(NOXAUDIO_PHONON_LIB "${NOXAUDIO_STEAMAUDIO_DIR}/lib/osx/libphonon.dylib")
else()
    set(NOXAUDIO_PHONON_LIB "${NOXAUDIO_STEAMAUDIO_DIR}/lib/linux-x64/libphonon.so")
endif()

if(NOT EXISTS "${NOXAUDIO_PHONON_LIB}")
    message(FATAL_ERROR "Steam Audio library not found at '${NOXAUDIO_PHONON_LIB}'. Run scripts/install_dependencies.bat first.")
endif()

add_library(SteamAudio::Phonon SHARED IMPORTED)
set_target_properties(SteamAudio::Phonon PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${NOXAUDIO_STEAMAUDIO_DIR}/include"
)
if(WIN32)
    set_target_properties(SteamAudio::Phonon PROPERTIES
        IMPORTED_IMPLIB "${NOXAUDIO_PHONON_LIB}"
        IMPORTED_LOCATION "${NOXAUDIO_PHONON_DLL}"
    )
else()
    set_target_properties(SteamAudio::Phonon PROPERTIES
        IMPORTED_LOCATION "${NOXAUDIO_PHONON_LIB}"
    )
endif()

# -----------------------------------------------------------------------------
# NoxAudio Static Library
# -----------------------------------------------------------------------------
file(GLOB_RECURSE NOXAUDIO_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/NoxAudio/*.h"
)
file(GLOB_RECURSE NOXAUDIO_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(NoxAudio STATIC ${NOXAUDIO_HEADERS} ${NOXAUDIO_SOURCES})
add_library(NoxAudio::Core ALIAS NoxAudio)
add_library(NoxAudio::NoxAudio ALIAS NoxAudio)

target_include_directories(NoxAudio
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${NOXAUDIO_MINIAUDIO_DIR}"
)

target_compile_features(NoxAudio PUBLIC cxx_std_20)
target_compile_definitions(NoxAudio
    PUBLIC
        GLM_FORCE_EXPLICIT_CTOR
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_ENABLE_EXPERIMENTAL
    PRIVATE
        MA_NO_ENCODING
        MA_NO_GENERATION
        MA_NO_ENGINE
        MA_NO_NODE_GRAPH
        MA_NO_RESOURCE_MANAGER
        $<$<BOOL:${WIN32}>:NOMINMAX>
        $<$<BOOL:${WIN32}>:WIN32_LEAN_AND_MEAN>
        _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(NoxAudio
    PUBLIC
        glm::glm
        SteamAudio::Phonon
    PRIVATE
        yaml-cpp::yaml-cpp
        $<$<BOOL:${WIN32}>:avrt>
)

# -----------------------------------------------------------------------------
# Standalone Verification Tests
# -----------------------------------------------------------------------------
if(NOXAUDIO_BUILD_TESTS)
    add_executable(NoxAudioTest tests/NoxAudioTest/main.cpp)
    target_link_libraries(NoxAudioTest PRIVATE NoxAudio::Core)
    target_compile_features(NoxAudioTest PRIVATE cxx_std_20)

    if(WIN32 AND EXISTS "${NOXAUDIO_PHONON_DLL}")
        add_custom_command(TARGET NoxAudioTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${NOXAUDIO_PHONON_DLL}"
                "$<TARGET_FILE_DIR:NoxAudioTest>"
        )
    endif()
endif()
