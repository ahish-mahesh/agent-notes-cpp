# 🎤 Agent Notes C++

An intelligent, real-time audio transcription and note-taking application with AI-powered summarization written in C++ using OpenAI's Whisper and Llama models.

## ✨ Features

- **🚀 Real-time Transcription**: Live speech-to-text with low latency using Whisper
- **🤖 AI Summarization**: Intelligent text summarization using Llama models (Qwen 2.5 0.5B)
- **💾 Database Persistence**: SQLite integration for storing transcriptions and summaries
- **🎯 High Performance**: Native C++ implementation with optimized audio processing
- **🔧 Cross-platform**: Supports macOS, Linux, and Windows
- **🎚️ Multiple Audio APIs**: RtAudio and PortAudio support
- **🌍 Multi-language**: Supports 99+ languages via Whisper
- **⚙️ Configurable**: Adjustable device selection, threading, and quality settings
- **📱 Device Management**: List and select audio input devices
- **🛡️ Robust**: Comprehensive error handling and graceful shutdown

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────────┐
│   Microphone    │───▶│   AudioCapture   │───▶│ WhisperTranscriber  │
└─────────────────┘    │  (RtAudio/PA)    │    │   (whisper.cpp)     │
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
│  (Persistence)  │    │   (Database)     │    │  (llama.cpp/Qwen)   │
└─────────────────┘    └──────────────────┘    └─────────────────────┘
                                 │                         │
                                 ▼                         ▼
                        ┌──────────────────┐    ┌─────────────────────┐
                        │  Stored Notes    │    │   AI Summaries      │
                        │ & Transcriptions │    │   [Smart Insights]  │
                        └──────────────────┘    └─────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- **macOS**: Xcode Command Line Tools, Homebrew
- **Linux**: GCC/Clang, CMake, ALSA development headers
- **Windows**: Visual Studio 2019+, CMake

### Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/ahish-mahesh/agent-notes-cpp.git
cd agent-notes-cpp

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Download Whisper model
curl -L -o ../ggml-base.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin

# Download Qwen 2.5 0.5B model for summarization
curl -L -o ../qwen2.5-0.5b-instruct-q4_0.gguf https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_0.gguf
```

## 🎮 Usage

### Basic Usage

```bash
# Start transcription and summarization
./build/agent-notes ggml-base.en.bin qwen2.5-0.5b-instruct-q4_0.gguf

# List available audio devices
./build/agent-notes --list-devices

# Use specific device
./build/agent-notes ggml-base.en.bin qwen2.5-0.5b-instruct-q4_0.gguf --device 1
```

### Expected Output

```
🎤 Agent Notes C++ v1.0.0
Intelligent audio transcription with AI summarization
═══════════════════════════════════════════════════

🤖 Loading Whisper model: ggml-base.en.bin
✅ Whisper model loaded successfully
🧠 Loading LLM model: qwen2.5-0.5b-instruct-q4_0.gguf
✅ LLM model loaded successfully
🗄️  Initializing database...
✅ Database initialized
🎙️  Initializing audio capture...
✅ Audio capture initialized

🎤 Listening... (Press Ctrl+C to stop)
═══════════════════════════════════

[14:30:15] I need to schedule a meeting with the team tomorrow
[14:30:18] Let's discuss the project roadmap and deliverables
[14:30:22] We should also review the budget allocation

🧠 AI Summary: Meeting planning discussion covering team scheduling, project roadmap review, and budget considerations for tomorrow's session.

💾 Saved to database: Transcription ID 1
```

## 📊 Performance

### Model Performance (MacBook Air M2)

| Component | Model | Size | Speed | Quality |
|-----------|-------|------|-------|---------|
| **Transcription** | Whisper base.en | 142MB | 16x realtime | Good ⭐ |
| **Summarization** | Qwen 2.5 0.5B | ~300MB | ~2-3s | Efficient ⭐ |

*⭐ Optimized for real-time performance and resource efficiency*

### System Requirements

- **CPU**: Any modern processor (Intel/AMD/Apple Silicon)
- **Memory**: 4GB+ available RAM (2GB for models + 2GB system)
- **Audio**: Working microphone or audio input device
- **Storage**: 1GB+ for models and database
- **OS**: macOS 10.15+, Linux (Ubuntu 18.04+), Windows 10+

## 🔧 Components

### Core Classes

- **`AudioCapture`**: Real-time audio input with optimized 128-frame buffer
- **`WhisperTranscriber`**: Speech-to-text via WhisperBridge API
- **`LLMClient`**: Text summarization using LlamaBridge API
- **`DBHelper`**: SQLite database operations for persistence

### Recent Optimizations

- **Buffer Size**: Reduced from 256 to 128 frames for lower latency
- **Model Switch**: Updated to Qwen 2.5 0.5B for efficient summarization
- **Build System**: Static linking of whisper.cpp and llama.cpp libraries
- **Prompt Engineering**: Enhanced summarization prompts for better results

## 📁 Project Structure

```
agent-notes-cpp/
├── 📁 include/                 # Header files
│   ├── AudioCapture.h         # Audio input interface  
│   ├── WhisperTranscriber.h   # Whisper wrapper
│   ├── LLMClient.h            # LLM summarization
│   ├── DBHelper.h             # Database operations
│   └── AudioBuffer.h          # Ring buffer
├── 📁 src/                    # Implementation files
│   ├── main.cpp              # Application entry point
│   ├── AudioCapture.cpp      # Audio capture implementation
│   ├── WhisperTranscriber.cpp# Whisper integration
│   ├── LLMClient.cpp         # LLM client implementation
│   └── DBHelper.cpp          # Database helper
├── 📁 third_party/           # Dependencies (git submodules)
│   ├── whisper.cpp/          # Whisper C++ implementation
│   └── llama.cpp/            # Llama C++ implementation
├── 📁 build/                 # Build artifacts
├── CMakeLists.txt            # Build configuration
└── README.md                 # This file
```

## 🛠️ Build Configuration

### CMake Options

```bash
# Debug build with symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Static library builds (default)
cmake .. -DUSE_STATIC_LIBS=ON
```

### Dependencies

- **whisper.cpp**: Speech recognition (git submodule)
- **llama.cpp**: LLM inference (git submodule)  
- **SQLite**: Database persistence (system library)
- **RtAudio/PortAudio**: Cross-platform audio I/O

## 🗄️ Database Schema

The application automatically creates SQLite tables for:

- **Transcriptions**: Audio transcription texts with timestamps
- **Summaries**: AI-generated summaries linked to transcriptions
- **Sessions**: Audio capture session metadata

## 🎯 Models

### Recommended Models

```bash
# Whisper models (speech-to-text)
curl -L -O https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin    # Recommended
curl -L -O https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin   # Higher quality

# Qwen models (text summarization) 
curl -L -O https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_0.gguf  # Current
curl -L -O https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q4_0.gguf  # More capable
```

## 🔧 Configuration

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
whisperConfig.modelPath = "ggml-base.en.bin";
whisperConfig.language = "auto";   // Auto-detect
whisperConfig.threads = 4;         // CPU cores

// LLM configuration  
LLMClient::Config llmConfig;
llmConfig.modelPath = "qwen2.5-0.5b-instruct-q4_0.gguf";
llmConfig.maxTokens = 512;         // Summary length
llmConfig.temperature = 0.3;       // Conservative generation
```

## 🔧 Troubleshooting

### Common Issues

**"Failed to load LLM model"**
```bash
# Verify model file exists and is valid
ls -la *.gguf

# Check model compatibility
./build/agent-notes --test-llm qwen2.5-0.5b-instruct-q4_0.gguf
```

**"Database initialization failed"**
```bash
# Check write permissions
touch test.db && rm test.db

# Verify SQLite installation
sqlite3 --version
```

**High memory usage**
```bash
# Use smaller models
./build/agent-notes ggml-tiny.en.bin qwen2.5-0.5b-instruct-q4_0.gguf

# Reduce model context size in config
```

## 🚀 Recent Updates

### Version History
- **Latest**: Model optimization with Qwen 2.5 0.5B, enhanced prompts
- **v0.9**: LLM summarization integration, static library builds  
- **v0.8**: Database persistence, DBHelper class implementation
- **v0.7**: Audio buffer optimization, reduced latency to 128 frames
- **v0.6**: WhisperBridge and LlamaBridge API integration
- **v0.5**: Initial LLM client implementation

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make changes and test thoroughly  
4. Commit with clear messages: `git commit -m "Add feature X"`
5. Push and create a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **[OpenAI](https://openai.com)** - For the Whisper model
- **[Alibaba Cloud](https://qwenlm.github.io/)** - For the Qwen language models
- **[ggerganov](https://github.com/ggerganov)** - For whisper.cpp and llama.cpp implementations
- **[thestk](https://github.com/thestk)** - For RtAudio library

## 📞 Support

- 🐛 **Bug Reports**: [GitHub Issues](../../issues)
- 💬 **Discussions**: [GitHub Discussions](../../discussions)  
- 📖 **Documentation**: [Wiki](../../wiki)

---

**Made with ❤️ and C++** | **Intelligent transcription and summarization for everyone** 🎤🧠✨