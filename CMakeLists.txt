cmake_minimum_required(VERSION 3.16)
project(AudioTranscriber VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Option to build with different audio libraries
option(USE_RTAUDIO "Use RtAudio for audio I/O" OFF)
option(USE_PORTAUDIO "Use PortAudio for audio I/O" ON)

# whisper.cpp configuration
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add whisper.cpp as subdirectory (git submodule)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp/CMakeLists.txt")
    add_subdirectory(third_party/whisper.cpp)
else()
    message(FATAL_ERROR "whisper.cpp not found. Please run: git submodule update --init --recursive")
endif()

# Audio library configuration
if(USE_RTAUDIO)
    # Add RtAudio
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rtaudio/CMakeLists.txt")
        add_subdirectory(third_party/rtaudio)
        set(AUDIO_LIBRARY rtaudio)
        add_definitions(-DUSE_RTAUDIO)
    else()
        message(FATAL_ERROR "RtAudio not found. Please run: git submodule update --init --recursive")
    endif()
elseif(USE_PORTAUDIO)
    # Find PortAudio
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
    set(AUDIO_LIBRARY ${PORTAUDIO_LIBRARIES})
    add_definitions(-DUSE_PORTAUDIO)
    include_directories(${PORTAUDIO_INCLUDE_DIRS})
    link_directories(${PORTAUDIO_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "Please enable either USE_RTAUDIO or USE_PORTAUDIO")
endif()

# Create executable
add_executable(audio-transcriber
    src/main.cpp
    src/AudioCapture.cpp
    src/WhisperTranscriber.cpp
    src/AudioBuffer.cpp
)

# Include directories
target_include_directories(audio-transcriber PRIVATE
    include
    third_party/whisper.cpp
    third_party/rtaudio
)

# Link libraries
target_link_libraries(audio-transcriber PRIVATE
    whisper
    ${AUDIO_LIBRARY}
)

# Platform-specific configurations
if(APPLE)
    target_link_libraries(audio-transcriber PRIVATE
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreFoundation"
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
    )
elseif(LINUX)
    target_link_libraries(audio-transcriber PRIVATE
        pthread
        alsa
    )
elseif(WIN32)
    target_link_libraries(audio-transcriber PRIVATE
        winmm
        dsound
        ole32
    )
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(audio-transcriber PRIVATE
        -Wall -Wextra -O2
    )
endif()

# Install target
install(TARGETS audio-transcriber
    RUNTIME DESTINATION bin
)

# Print build configuration
message(STATUS "Build configuration:")
message(STATUS "  Audio library: ${AUDIO_LIBRARY}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")