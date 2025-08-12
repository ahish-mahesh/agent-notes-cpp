cmake_minimum_required(VERSION 3.16)
project(AudioTranscriber VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Include ExternalProject module
include(ExternalProject)

# Option to build with different audio libraries
option(USE_RTAUDIO "Use RtAudio for audio I/O" OFF)
option(USE_PORTAUDIO "Use PortAudio for audio I/O" ON)

# Create separate install directories for each external project
set(WHISPER_INSTALL_DIR "${CMAKE_BINARY_DIR}/whisper-install")
set(LLAMA_INSTALL_DIR "${CMAKE_BINARY_DIR}/llama-install")

# Build whisper.cpp as external project with static libraries
ExternalProject_Add(whisper_external
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${WHISPER_INSTALL_DIR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DWHISPER_BUILD_TESTS=OFF
        -DWHISPER_BUILD_EXAMPLES=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_CXX_STANDARD=17
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
    BUILD_BYPRODUCTS
        ${WHISPER_INSTALL_DIR}/lib/libwhisper.a
        ${WHISPER_INSTALL_DIR}/lib/libggml.a
        ${WHISPER_INSTALL_DIR}/lib/libggml-base.a
        ${WHISPER_INSTALL_DIR}/lib/libggml-cpu.a
        ${WHISPER_INSTALL_DIR}/lib/libggml-metal.a
        ${WHISPER_INSTALL_DIR}/lib/libggml-blas.a
)

# Build llama.cpp as external project with static libraries
ExternalProject_Add(llama_external
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${LLAMA_INSTALL_DIR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DLLAMA_BUILD_TESTS=OFF
        -DLLAMA_BUILD_EXAMPLES=OFF
        -DLLAMA_BUILD_SERVER=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_CXX_STANDARD=17
        -DCMAKE_CXX_STANDARD_REQUIRED=ON
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
    BUILD_BYPRODUCTS
        ${LLAMA_INSTALL_DIR}/lib/libllama.a
        ${LLAMA_INSTALL_DIR}/lib/libggml.a
        ${LLAMA_INSTALL_DIR}/lib/libggml-base.a
        ${LLAMA_INSTALL_DIR}/lib/libggml-cpu.a
        ${LLAMA_INSTALL_DIR}/lib/libggml-metal.a
        ${LLAMA_INSTALL_DIR}/lib/libggml-blas.a
)

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

# ============================================================================
# WRAPPER LIBRARIES (Isolated header includes)
# ============================================================================

# Whisper Wrapper Library - Only this library includes whisper headers
add_library(whisper_wrapper SHARED
    src/WhisperBridge.cpp
)

add_dependencies(whisper_wrapper whisper_external)

# PRIVATE includes - whisper headers are isolated to this library
target_include_directories(whisper_wrapper PRIVATE
    include
    ${WHISPER_INSTALL_DIR}/include
)

# Static whisper linking (cross-platform) with all GGML components
target_link_libraries(whisper_wrapper PRIVATE
    ${WHISPER_INSTALL_DIR}/lib/libwhisper.a
    ${WHISPER_INSTALL_DIR}/lib/libggml.a
    ${WHISPER_INSTALL_DIR}/lib/libggml-base.a
    ${WHISPER_INSTALL_DIR}/lib/libggml-cpu.a
    ${WHISPER_INSTALL_DIR}/lib/libggml-metal.a
    ${WHISPER_INSTALL_DIR}/lib/libggml-blas.a
)

# Link system frameworks required by GGML on macOS
if(APPLE)
    target_link_libraries(whisper_wrapper PRIVATE
        "-framework Metal"
        "-framework MetalKit"
        "-framework Accelerate"
        "-framework Foundation"
        "-framework CoreFoundation"
    )
endif()

# No RPATH needed for static linking
# Static libraries are embedded at link time

# Llama Wrapper Library - Only this library includes llama headers
add_library(llama_wrapper SHARED
    src/LlamaBridge.cpp
)

add_dependencies(llama_wrapper llama_external)

# PRIVATE includes - llama headers are isolated to this library
target_include_directories(llama_wrapper PRIVATE
    include
    ${LLAMA_INSTALL_DIR}/include
)

# Static llama linking (cross-platform) with all GGML components
target_link_libraries(llama_wrapper PRIVATE
    ${LLAMA_INSTALL_DIR}/lib/libllama.a
    ${LLAMA_INSTALL_DIR}/lib/libggml.a
    ${LLAMA_INSTALL_DIR}/lib/libggml-base.a
    ${LLAMA_INSTALL_DIR}/lib/libggml-cpu.a
    ${LLAMA_INSTALL_DIR}/lib/libggml-metal.a
    ${LLAMA_INSTALL_DIR}/lib/libggml-blas.a
)

# Link system frameworks required by GGML on macOS
if(APPLE)
    target_link_libraries(llama_wrapper PRIVATE
        "-framework Metal"
        "-framework MetalKit"
        "-framework Accelerate"
        "-framework Foundation"
        "-framework CoreFoundation"
    )
endif()

# No RPATH needed for static linking
# Static libraries are embedded at link time

# ============================================================================
# MAIN EXECUTABLE (No direct whisper/llama/ggml exposure)
# ============================================================================

# Create main executable
add_executable(audio-transcriber
    src/main.cpp
    src/AudioCapture.cpp
    src/WhisperTranscriber.cpp
    src/AudioBuffer.cpp
    src/DBHelper.cpp
    src/LLMClient.cpp
)

# Make executable depend on wrapper libraries
add_dependencies(audio-transcriber whisper_wrapper llama_wrapper)

# SAFE includes - only our own headers and system headers
target_include_directories(audio-transcriber PRIVATE
    include
    third_party/rtaudio
    ${SQLITE3_INCLUDE_DIRS}
)

# Link against wrapper libraries (NOT directly against whisper/llama)
target_link_libraries(audio-transcriber PRIVATE
    whisper_wrapper
    llama_wrapper
    ${AUDIO_LIBRARY}
    ${SQLITE3_LIBRARIES}
)

# Set RPATH so executable can find wrapper libraries at runtime
set_target_properties(audio-transcriber PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR}"
    INSTALL_RPATH "${CMAKE_BINARY_DIR}"
    BUILD_RPATH_USE_ORIGIN TRUE
    INSTALL_RPATH_USE_LINK_PATH TRUE
    MACOSX_RPATH TRUE
)

# Add platform-specific system frameworks and libraries
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
else() # Linux
    target_link_libraries(audio-transcriber PRIVATE
        pthread
        alsa
    )
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(audio-transcriber PRIVATE
        -Wall -Wextra -O2
    )
    target_compile_options(whisper_wrapper PRIVATE
        -Wall -Wextra -O2
    )
    target_compile_options(llama_wrapper PRIVATE
        -Wall -Wextra -O2
    )
endif()

# Install target
install(TARGETS audio-transcriber whisper_wrapper llama_wrapper
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# Print build configuration
message(STATUS "Build configuration:")
message(STATUS "  Audio library: ${AUDIO_LIBRARY}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Whisper wrapper: Isolated includes")
message(STATUS "  Llama wrapper: Isolated includes")
message(STATUS "  Main executable: No direct ggml exposure")