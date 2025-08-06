# 🎤 Audio Transcriber

A high-performance, real-time audio transcription application written in C++ using OpenAI's Whisper model and modern audio libraries.

## ✨ Features

- **🚀 Real-time Transcription**: Live speech-to-text with low latency
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
                       │   AudioBuffer    │    │   Console Output    │
                       │ (Ring Buffer)    │    │   [HH:MM:SS] Text   │
                       └──────────────────┘    └─────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- **macOS**: Xcode Command Line Tools, Homebrew
- **Linux**: GCC/Clang, CMake, ALSA development headers
- **Windows**: Visual Studio 2019+, CMake

### One-Command Setup

```bash
# Clone and setup everything
git clone <your-repo-url> audio-transcriber
cd audio-transcriber
chmod +x setup.sh && ./setup.sh
```

The setup script will:
- Initialize git submodules (whisper.cpp, RtAudio)
- Install system dependencies
- Build the project
- Download the base Whisper model
- Create run scripts

### Manual Setup

```bash
# 1. Clone with submodules
git clone --recursive <repo-url>
cd audio-transcriber

# 2. Create build directory
mkdir build && cd build

# 3. Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_RTAUDIO=ON
make -j$(nproc)

# 4. Download a model
curl -L -o ../ggml-base.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
```

## 🎮 Usage

### Basic Usage

```bash
# Start with default settings
./run.sh

# Or directly
./build/audio-transcriber ggml-base.en.bin
```

### Advanced Usage

```bash
# List available audio devices
./build/audio-transcriber --list-devices

# Use specific device and language
./build/audio-transcriber ggml-base.en.bin --device 1 --language en

# Multi-threading for better performance
./build/audio-transcriber ggml-small.en.bin --threads 8

# See all options
./build/audio-transcriber --help
```

### Expected Output

```
🎤 Audio Transcriber v1.0.0
Real-time speech transcription using Whisper
═══════════════════════════════════════════

🤖 Loading Whisper model: ggml-base.en.bin
✅ Whisper model loaded successfully
🎙️  Initializing audio capture...
🎧 Using audio device: MacBook Air Microphone
✅ Audio capture initialized

🎤 Listening... (Press Ctrl+C to stop)
═══════════════════════════════════

[14:30:15.123] Hello, this is a test of the audio transcription system.
[14:30:18.456] The weather is nice today and I'm testing the accuracy.
[14:30:22.789] The system seems to be working quite well with low latency.
```

## 📊 Performance

### Benchmarks (MacBook Air M2)

| Model | Size | Speed | Latency | Quality |
|-------|------|-------|---------|---------|
| tiny.en | 39MB | 32x realtime | ~100ms | Basic |
| base.en | 142MB | 16x realtime | ~200ms | Good ⭐ |
| small.en | 244MB | 6x realtime | ~500ms | Better |
| medium.en | 769MB | 2x realtime | ~1.5s | High |

*⭐ Recommended for real-time use*

### System Requirements

- **CPU**: Any modern processor (Intel/AMD/Apple Silicon)
- **Memory**: 2GB+ available RAM
- **Audio**: Working microphone or audio input device
- **OS**: macOS 10.15+, Linux (Ubuntu 18.04+), Windows 10+

## ⚙️ Configuration

### Audio Settings

```cpp
AudioCapture::Config audioConfig;
audioConfig.sampleRate = 16000;    // Whisper requirement
audioConfig.channels = 1;          // Mono
audioConfig.bufferSize = 256;      // Lower = less latency
audioConfig.deviceId = 0;          // 0 = default device
```

### Whisper Settings

```cpp
WhisperTranscriber::Config whisperConfig;
whisperConfig.modelPath = "ggml-base.en.bin";
whisperConfig.language = "auto";   // Auto-detect or specify
whisperConfig.threads = 4;         // CPU cores to use
whisperConfig.enableVAD = true;    // Voice Activity Detection
```

## 📁 Project Structure

```
audio-transcriber/
├── 📁 include/                 # Header files
│   ├── AudioCapture.h         # Audio input interface
│   ├── WhisperTranscriber.h   # Whisper wrapper
│   └── AudioBuffer.h          # Ring buffer
├── 📁 src/                    # Implementation files
│   ├── main.cpp              # Application entry point
│   ├── AudioCapture.cpp      # Audio capture implementation
│   ├── WhisperTranscriber.cpp# Whisper integration
│   └── AudioBuffer.cpp       # Buffer implementation
├── 📁 third_party/           # Dependencies (git submodules)
│   ├── whisper.cpp/          # Whisper C++ implementation
│   └── rtaudio/              # RtAudio library
├── 📁 build/                 # Build artifacts
├── CMakeLists.txt            # Build configuration
├── setup.sh                  # Setup script
└── run.sh                    # Quick run script
```

## 🛠️ Build Options

### Audio Library Selection

```bash
# Use RtAudio (default, recommended)
cmake .. -DUSE_RTAUDIO=ON

# Use PortAudio instead
cmake .. -DUSE_PORTAUDIO=ON
```

### Build Types

```bash
# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Debug build (with symbols)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# With additional optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"
```

## 🎯 Models

### Download Models

```bash
# English-only models (smaller, faster)
curl -L -O https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin
curl -L -O https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
curl -L -O https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin

# Multilingual models
curl -L -O https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin
curl -L -O https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin
```

### Model Comparison

| Model | Parameters | English | Multilingual | Best For |
|-------|------------|---------|--------------|----------|
| tiny | 39M | ✅ | ✅ | Testing |
| base | 74M | ✅ | ✅ | **Real-time** |
| small | 244M | ✅ | ✅ | **Balanced** |
| medium | 769M | ✅ | ✅ | **High quality** |
| large | 1550M | ❌ | ✅ | Best quality |

## 🔧 Troubleshooting

### Common Issues

**"No audio devices found"**
```bash
# Check audio permissions
./build/audio-transcriber --list-devices

# On macOS, grant microphone permissions:
# System Preferences > Security & Privacy > Privacy > Microphone
```

**"Failed to load Whisper model"**
```bash
# Verify model file exists and is valid
ls -la *.bin

# Re-download if corrupted
rm ggml-base.en.bin
curl -L -O https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
```

**High CPU usage**
```bash
# Use smaller model
./build/audio-transcriber ggml-tiny.en.bin --threads 2

# Reduce threads
./build/audio-transcriber ggml-base.en.bin --threads 1
```

**Audio dropouts/stuttering**
```bash
# Increase buffer size in AudioCapture::Config
# Edit src/AudioCapture.cpp and rebuild
```

### Debug Build

```bash
# Build with debug symbols
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run with debugger
gdb ./audio-transcriber
(gdb) run ggml-base.en.bin
```

### Verbose Logging

```bash
# Enable verbose whisper.cpp output
export WHISPER_LOG_LEVEL=1
./build/audio-transcriber ggml-base.en.bin
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make changes and test thoroughly
4. Commit with clear messages: `git commit -m "Add feature X"`
5. Push and create a pull request

### Development Setup

```bash
# Install development tools
brew install clang-format cmake-format

# Format code before committing
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Run tests
cd build && make test
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **[OpenAI](https://openai.com)** - For the Whisper model
- **[ggerganov](https://github.com/ggerganov)** - For whisper.cpp implementation  
- **[thestk](https://github.com/thestk)** - For RtAudio library
- **[PortAudio](http://www.portaudio.com/)** - For the PortAudio library

## 📞 Support

- 🐛 **Bug Reports**: [GitHub Issues](../../issues)
- 💬 **Discussions**: [GitHub Discussions](../../discussions)
- 📖 **Documentation**: [Wiki](../../wiki)

---

**Made with ❤️ and C++** | **Real-time transcription for everyone** 🎤✨