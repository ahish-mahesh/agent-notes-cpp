# Agent Notes C++

An intelligent, real-time audio transcription and note-taking application with AI-powered summarization written in C++ using OpenAI's Whisper and Llama models.

## Features

- ** Real-time Transcription**: Live speech-to-text with low latency using Whisper
- **AI Summarization**: Intelligent text summarization using Llama models (Qwen 2.5 0.5B)
- **Database Persistence**: SQLite integration for storing transcriptions and summaries
- **High Performance**: Native C++ implementation with optimized audio processing
- **Cross-platform**: Supports macOS, Linux, and Windows
- **Multiple Audio APIs**: RtAudio and PortAudio support
- **Multi-language**: Supports 99+ languages via Whisper
- **Configurable**: Adjustable device selection, threading, and quality settings
- **Device Management**: List and select audio input devices
- **Robust**: Comprehensive error handling and graceful shutdown

## Architecture

```t
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────────┐
│   Microphone    │───▶│   AudioCapture   │───▶│ WhisperTranscriber  │
└─────────────────┘    │  (RtAudio/PA)    │    │  (WhisperBridge)    │
                        └──────────────────┘    └─────────────────────┘
                                 │                         │
                                 ▼                         ▼
                        ┌──────────────────┐    ┌─────────────────────┐
                        │   AudioBuffer    │    │    Transcription    │
                        │ (Ring Buffer)    │    │       Text          │
                        └──────────────────┘    └─────────────────────┘
                                                           │
                                                           ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────────┐
│   SQLite DB     │◀───│    DBHelper      │◀───│     LLMClient       │
│  (Persistence)  │    │   (Database)     │    │  (LlamaBridge/Qwen) │
└─────────────────┘    └──────────────────┘    └─────────────────────┘
                                 │                         │
                                 ▼                         ▲
                        ┌──────────────────┐    ┌─────────────────────┐
                        │  Stored Notes    │    │    LlamaServer      │
                        │ & Transcriptions │    │ (Background Process)│
                        └──────────────────┘    └─────────────────────┘
                                 │                         │
                                 ▼                         ▼
                        ┌──────────────────┐    ┌─────────────────────┐
                        │  AgentNotesCli   │    │   AI Summaries      │
                        │ (User Interface) │    │   [Smart Insights]  │
                        └──────────────────┘    └─────────────────────┘
```

## Quick Start

### Prerequisites

- **macOS**: Xcode Command Line Tools, Homebrew
- **Linux**: GCC/Clang, CMake, ALSA development headers
- **Windows**: Visual Studio 2019+, CMake

### Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/ahish-mahesh/agent-notes-backend.git
cd agent-notes-backend

# Complete setup with dependencies, build, and model download
./setup.sh

# Alternative: Manual setup
# mkdir build && cd build
# cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_PORTAUDIO=ON
# make -j$(nproc)  # Linux
# make -j$(sysctl -n hw.ncpu)  # macOS
```

## Usage

### Basic Usage

```bash
# Quick start with run script
./run.sh

# Start with specific models and options
./build/audio-transcriber Models/ggml-base.en.bin Models/qwen2.5-0.5b-instruct-q4_k_m.gguf

# List available audio devices
./build/audio-transcriber --list-devices

# Use specific device
./build/audio-transcriber Models/ggml-base.en.bin Models/qwen2.5-0.5b-instruct-q4_k_m.gguf --device 1 --language en --threads 8
```

### Expected Output

```t
Agent Notes CLI v1.0.0
Intelligent audio transcription with AI summarization
═══════════════════════════════════════════════════

Loading Whisper model: Models/ggml-base.en.bin
Whisper model loaded successfully
Loading LLM model: Models/qwen2.5-0.5b-instruct-q4_k_m.gguf
LLM model loaded successfully
Initializing database...
Database initialized
Initializing audio capture...
Audio capture initialized

Welcome to Agent Notes CLI!
Type 'help' for available commands.

> help
Available commands:
  transcribe    - Start real-time transcription
  notes         - List all notes
  new-note      - Create a new note
  delete-note   - Delete a note by ID
  chats         - List all chats
  new-chat      - Start a new chat
  delete-chat   - Delete a chat by ID
  quit          - Exit the application

> transcribe
Listening... (Press 'q' + Enter to stop transcription)

[14:30:15] I need to schedule a meeting with the team tomorrow
[14:30:18] Let's discuss the project roadmap and deliverables
[14:30:22] We should also review the budget allocation

Generating AI Summary...
AI Summary: Meeting planning discussion covering team scheduling, project roadmap review, and budget considerations for tomorrow's session.

Saved to database: Transcription ID 1

> quit
Goodbye! Thanks for using Agent Notes CLI.
```

## Performance

### Model Performance (MacBook Air M2)

| Component | Model | Size | Speed | Quality |
|-----------|-------|------|-------|---------|
| **Transcription** | Whisper base.en | 142MB | 16x realtime | Good |
| **Summarization** | Qwen 2.5 0.5B | ~300MB | ~2-3s | Efficient |

### System Requirements

- **CPU**: Any modern processor (Intel/AMD/Apple Silicon)
- **Memory**: 4GB+ available RAM (2GB for models + 2GB system)
- **Audio**: Working microphone or audio input device
- **Storage**: 1GB+ for models and database
- **OS**: macOS 10.15+, Linux (Ubuntu 18.04+), Windows 10+

## Components

### Core Classes

- **`AudioCapture`**: Real-time audio input with optimized 128-frame buffer
- **`WhisperTranscriber`**: Speech-to-text via WhisperBridge API
- **`LLMClient`**: Text summarization using LlamaBridge API
- **`LlamaServer`**: Background llama.cpp server process management
- **`AgentNotesCli`**: Command-line interface for interactive note-taking
- **`DBHelper`**: SQLite database operations for persistence

### Recent Optimizations

- **Buffer Size**: Reduced from 256 to 128 frames for lower latency
- **Model Switch**: Updated to Qwen 2.5 0.5B for efficient summarization
- **Build System**: Static linking of whisper.cpp and llama.cpp libraries
- **Prompt Engineering**: Enhanced summarization prompts for better results

## Project Structure

```t
agent-notes-backend/
├── include/                 # Header files
│   ├── AudioCapture.h         # Audio input interface  
│   ├── WhisperTranscriber.h   # Whisper wrapper
│   ├── WhisperBridge.h        # Whisper C++ bridge
│   ├── LLMClient.h            # LLM summarization
│   ├── LlamaBridge.h          # Llama C++ bridge
│   ├── LlamaServer.h          # Background server management
│   ├── AgentNotesCli.h        # CLI interface
│   ├── DBHelper.h             # Database operations
│   └── AudioBuffer.h          # Ring buffer
├── src/                    # Implementation files
│   ├── main.cpp              # Application entry point
│   ├── AgentNotesCli.cpp     # CLI interface implementation
│   ├── AudioCapture.cpp      # Audio capture implementation
│   ├── WhisperTranscriber.cpp# Whisper integration
│   ├── WhisperBridge.cpp     # Whisper C++ bridge
│   ├── LLMClient.cpp         # LLM client implementation
│   ├── LlamaBridge.cpp       # Llama C++ bridge
│   ├── LlamaServer.cpp       # Background server management
│   └── DBHelper.cpp          # Database helper
├── Models/                 # AI model files
│   ├── ggml-base.en.bin      # Whisper model (English)
│   ├── ggml-tiny.en.bin      # Whisper model (Tiny)
│   └── qwen2.5-0.5b-instruct-q4_k_m.gguf  # Qwen LLM model
├── third_party/           # Dependencies (git submodules)
│   ├── whisper.cpp/          # Whisper C++ implementation
│   ├── llama.cpp/            # Llama C++ implementation
│   ├── rtaudio/              # RtAudio library
│   └── websocketpp/          # WebSocket++ library
├── build/                 # Build artifacts
├── CMakeLists.txt            # Build configuration
├── setup.sh                  # Setup script
├── run.sh                    # Quick run script
├── CLAUDE.md                 # Project instructions for Claude
└── README.md                 # This file
```

## Build Configuration

### CMake Options

```bash
# Debug build with symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_PORTAUDIO=ON

# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_PORTAUDIO=ON

# Use RtAudio instead of PortAudio
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_RTAUDIO=ON

# Build with all optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_PORTAUDIO=ON
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS
```

### Dependencies

- **whisper.cpp**: Speech recognition (git submodule)
- **llama.cpp**: LLM inference (git submodule)  
- **rtaudio**: Cross-platform audio I/O (git submodule, optional)
- **websocketpp**: WebSocket library (git submodule)
- **SQLite**: Database persistence (system library)
- **PortAudio**: Cross-platform audio I/O (system library, optional)
- **nlohmann/json**: JSON parsing (system library)
- **CURL**: HTTP client (system library)

## Database Schema

The application automatically creates SQLite tables for:

- **Transcriptions**: Audio transcription texts with timestamps
- **Summaries**: AI-generated summaries linked to transcriptions
- **Sessions**: Audio capture session metadata

## Models

### Recommended Models

```bash
# Whisper models (speech-to-text) - Download to Models/ directory
curl -L -o Models/ggml-base.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin    # Recommended
curl -L -o Models/ggml-small.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin   # Higher quality

# Qwen models (text summarization) - Download to Models/ directory
curl -L -o Models/qwen2.5-0.5b-instruct-q4_k_m.gguf https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf  # Current
curl -L -o Models/qwen2.5-1.5b-instruct-q4_0.gguf https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q4_0.gguf  # More capable
```

## Configuration

### Audio Settings

```cpp
AudioCapture::Config audioConfig;
audioConfig.sampleRate = 16000;    // Whisper requirement
audioConfig.channels = 1;          // Mono
audioConfig.bufferSize = 128;      // Optimized for low latency
audioConfig.deviceId = 0;          // 0 = default device
```

### AI Model Settings

```cpp
// Whisper configuration
WhisperTranscriber::Config whisperConfig;
whisperConfig.modelPath = "Models/ggml-base.en.bin";
whisperConfig.language = "auto";   // Auto-detect
whisperConfig.threads = 4;         // CPU cores

// LLM configuration  
LLMClient::Config llmConfig;
llmConfig.modelPath = "Models/qwen2.5-0.5b-instruct-q4_k_m.gguf";
llmConfig.maxTokens = 512;         // Summary length
llmConfig.temperature = 0.3;       // Conservative generation
```

## Troubleshooting

### Common Issues

#### "Failed to load LLM model"

```bash
# Verify model files exist and are valid
ls -la Models/*.gguf Models/*.bin

# Check model compatibility
./build/audio-transcriber --test-llm Models/qwen2.5-0.5b-instruct-q4_k_m.gguf
```

### "Database initialization failed"

```bash
# Check write permissions
touch test.db && rm test.db

# Verify SQLite installation
sqlite3 --version
```

### High memory usage

```bash
# Use smaller models
./build/audio-transcriber Models/ggml-tiny.en.bin Models/qwen2.5-0.5b-instruct-q4_k_m.gguf

# Reduce model context size in config
```

## Recent Updates

### Version History

- **Latest**: Model optimization with Qwen 2.5 0.5B, enhanced prompts
- **v0.9**: LLM summarization integration, static library builds  
- **v0.8**: Database persistence, DBHelper class implementation
- **v0.7**: Audio buffer optimization, reduced latency to 128 frames
- **v0.6**: WhisperBridge and LlamaBridge API integration
- **v0.5**: Initial LLM client implementation

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make changes and test thoroughly  
4. Commit with clear messages: `git commit -m "Add feature X"`
5. Push and create a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **[OpenAI](https://openai.com)** - For the Whisper model
- **[Alibaba Cloud](https://qwenlm.github.io/)** - For the Qwen language models
- **[ggerganov](https://github.com/ggerganov)** - For whisper.cpp and llama.cpp implementations
- **[thestk](https://github.com/thestk)** - For RtAudio library

## Support

- **Bug Reports**: [GitHub Issues](../../issues)
- **Discussions**: [GitHub Discussions](../../discussions)  
- **Documentation**: [Wiki](../../wiki)

---

**Made with C++** | **Intelligent transcription and summarization for everyone**
