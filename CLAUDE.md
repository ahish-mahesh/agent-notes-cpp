# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C++ real-time audio transcription application that captures audio from a microphone and transcribes it using OpenAI's Whisper model. The application stores transcriptions in a SQLite database and is designed for cross-platform compatibility (macOS, Linux, Windows).

## Build Commands

### Setup (First Time Only)
```bash
# Complete setup with dependencies, build, and model download
./setup.sh
```

### Manual Build
```bash
# Create and enter build directory
mkdir -p build && cd build

# Configure with CMake (choose audio library)
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_PORTAUDIO=ON
# OR for RtAudio: cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_RTAUDIO=ON

# Build the project
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS
```

### Running the Application
```bash
# Quick run with defaults
./run.sh

# Direct execution
./build/audio-transcriber ggml-base.en.bin [options]

# List available audio devices
./build/audio-transcriber --list-devices

# Run with specific device and options
./build/audio-transcriber ggml-base.en.bin --device 1 --language en --threads 8
```

### Development Commands
```bash
# Debug build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Clean rebuild
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_PORTAUDIO=ON
make -j$(nproc)
```

## High-Level Architecture

### Core Components

1. **AudioCapture** (`include/AudioCapture.h`, `src/AudioCapture.cpp`)
   - Handles real-time audio input from microphone
   - Supports both RtAudio and PortAudio backends
   - Converts audio to Whisper-compatible format (16kHz, mono, float32)
   - Thread-safe audio callbacks with ring buffering

2. **WhisperTranscriber** (`include/WhisperTranscriber.h`, `src/WhisperTranscriber.cpp`)
   - Wraps whisper.cpp for speech-to-text processing
   - Manages real-time audio buffer accumulation and processing
   - Configurable voice activity detection and language settings
   - Thread-safe queuing system for continuous transcription

3. **DBHelper** (`include/DBHelper.h`, `src/DBHelper.cpp`)
   - SQLite database interface for storing transcription results
   - Handles transcript persistence and retrieval

4. **AudioBuffer** (`include/AudioBuffer.h`, `src/AudioBuffer.cpp`)
   - Ring buffer implementation for audio data management
   - Thread-safe circular buffer operations

### Data Flow Architecture
```
Microphone → AudioCapture → AudioBuffer → WhisperTranscriber → Console Output
                                                    ↓
                                               DBHelper → SQLite DB
```

### Threading Model
- **Main Thread**: Application lifecycle, signal handling, user interface
- **Audio Thread**: Real-time audio capture callback (high priority)
- **Processing Thread**: Whisper inference and transcription processing
- **Buffer Management**: Ring buffer handles thread synchronization

## Dependencies and Third-Party Libraries

### Git Submodules (in `third_party/`)
- **whisper.cpp**: OpenAI Whisper C++ implementation
- **rtaudio**: Cross-platform audio I/O library (optional)

### System Dependencies
- **SQLite3**: Database storage (system package)
- **PortAudio**: Audio I/O library (system package, optional)
- **Platform Libraries**: CoreAudio (macOS), ALSA (Linux), WinMM (Windows)

## Configuration Options

### Audio Backend Selection (CMake)
- `-DUSE_RTAUDIO=ON`: Use RtAudio for audio I/O
- `-DUSE_PORTAUDIO=ON`: Use PortAudio for audio I/O

### Runtime Configuration
- Audio device selection via `--device` flag
- Language detection/specification via `--language` flag
- Threading control via `--threads` flag
- Model selection via command line argument

## Database Schema

### Tables
- **Transcripts**: `transcript_id` (int), `transcript_text` (nvarchar)
- **Chats**: Planned future feature for chat support

## Key Development Notes

- C++23 standard required
- Cross-platform build system using CMake
- Real-time audio processing with low-latency requirements
- Voice Activity Detection (VAD) for efficient processing
- Whisper model files (.bin) downloaded separately and not committed to repo
- Graceful shutdown handling with signal processing
- Thread-safe design with proper synchronization primitives
- Error handling and device enumeration for audio setup

## Model Management

Models are downloaded from Hugging Face and stored in project root:
- `ggml-base.en.bin`: Default English-only model (recommended for real-time)
- Other models available: tiny, small, medium, large variants
- Models are large binary files and excluded from git via .gitignore