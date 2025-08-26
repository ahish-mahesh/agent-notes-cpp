#!/bin/bash

# Audio Transcriber Setup Script
# This script sets up the complete C++ audio transcription project

set -e

echo "🎤 Audio Transcriber Setup"
echo "=========================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    print_error "This setup script is designed for macOS. Please adapt for your platform."
    exit 1
fi

print_status "Setting up Audio Transcriber project..."

# 1. Initialize git repository if not already done
if [ ! -d ".git" ]; then
    print_status "Initializing git repository..."
    git init
    print_success "Git repository initialized"
fi

# 2. Create .gitignore if it doesn't exist
if [ ! -f ".gitignore" ]; then
    print_status "Creating .gitignore..."
    cat > .gitignore << 'EOF'
# Build directories
build/
build-*/
bin/
lib/

# CMake
CMakeCache.txt
CMakeFiles/
CMakeScripts/
Testing/
Makefile
cmake_install.cmake
install_manifest.txt
compile_commands.json
CTestTestfile.cmake
_deps

# IDE files
.vscode/
.idea/
*.swp
*.swo
*~

# macOS
.DS_Store
.AppleDouble
.LSOverride

# Model files (they're large)
*.bin
*.ggml

# Temporary files
*.tmp
*.log
EOF
    print_success ".gitignore created"
fi

# 3. Add git submodules for dependencies
print_status "Adding git submodules..."

# Create third_party directory
mkdir -p third_party

# Add whisper.cpp
if [ ! -d "third_party/whisper.cpp" ]; then
    print_status "Adding whisper.cpp submodule..."
    git submodule add https://github.com/ggerganov/whisper.cpp.git third_party/whisper.cpp
    print_success "whisper.cpp submodule added"
else
    print_status "whisper.cpp submodule already exists"
fi

# Add RtAudio
if [ ! -d "third_party/rtaudio" ]; then
    print_status "Adding RtAudio submodule..."
    git submodule add https://github.com/thestk/rtaudio.git third_party/rtaudio
    print_success "RtAudio submodule added"
else
    print_status "RtAudio submodule already exists"
fi

# Add websocketpp
if [ ! -d "third_party/websocketpp" ]; then
    print_status "Adding websocketpp submodule..."
    git submodule add https://github.com/zaphoyd/websocketpp.git third_party/websocketpp
    print_success "websocketpp submodule added"
else
    print_status "websocketpp submodule already exists"
fi

# Update submodules
print_status "Updating submodules..."
git submodule update --init --recursive
print_success "Submodules updated"

# 4. Install system dependencies using Homebrew
print_status "Checking for Homebrew..."
if ! command -v brew &> /dev/null; then
    print_warning "Homebrew not found. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

print_status "Installing system dependencies..."
brew install cmake pkg-config

# 5. Check for required tools
print_status "Checking for required tools..."

if ! command -v cmake &> /dev/null; then
    print_error "CMake not found. Please install CMake."
    exit 1
fi

if ! command -v make &> /dev/null; then
    print_error "Make not found. Please install build tools."
    exit 1
fi

print_success "All required tools found"

# 6. Create build directory
print_status "Creating build directory..."
mkdir -p build
cd build

# 7. Configure with CMake
print_status "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_PORTAUDIO=ON

print_success "Project configured successfully"

# 8. Build the project
print_status "Building project..."
make -j$(sysctl -n hw.ncpu)

print_success "Project built successfully"

cd ..

# Create the Models Folder
print_status "Creating models directory..."
mkdir -p models

# 9. Download a basic model
print_status "Downloading Whisper base model..."
if [ ! -f "models/ggml-base.en.bin" ]; then
    curl -L -o models/ggml-base.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
    print_success "Base model downloaded (models/ggml-base.en.bin)"
else
    print_status "Base model already exists"
fi

# 9.b Download a llama-server model for local testing (qwen2.5-0.5b-instruct)
print_status "Downloading llama-server model..."
MODEL_DIR="models"
MODEL_FILE="$MODEL_DIR/qwen2.5-0.5b-instruct-q4_k_m.gguf"
if [ ! -f "$MODEL_FILE" ]; then
    print_status "Creating models directory..."
    mkdir -p "$MODEL_DIR"
    print_status "Downloading model to $MODEL_FILE (this may take a while)..."
    # Attempt to download from Hugging Face; if it fails, notify the user so they can download manually
    if curl -L --fail -o "$MODEL_FILE" "https://huggingface.co/qwen/qwen2.5-0.5b-instruct-q4_k_m/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf"; then
        print_success "Model downloaded ($MODEL_FILE)"
    else
        print_warning "Automatic download failed. Please download the model manually from Hugging Face and place it at $MODEL_FILE"
    fi
else
    print_status "Model already exists: $MODEL_FILE"
fi

# 10. Create a simple run script
print_status "Creating run script..."
cat > run.sh << 'EOF'
#!/bin/bash

# Simple run script for Audio Transcriber

# Start the llama-server
llama-server --model $MODEL_FILE &

if [ ! -f "build/audio-transcriber" ]; then
    echo "❌ Binary not found. Please run ./setup.sh first"
    exit 1
fi

MODEL_FILE="models/ggml-base.en.bin"
if [ ! -f "$MODEL_FILE" ]; then
    echo "❌ Model file not found: $MODEL_FILE"
    echo "Please download a model or run ./setup.sh"
    exit 1
fi

echo "🎤 Starting Audio Transcriber..."
echo "Press Ctrl+C to stop"
echo

./build/audio-transcriber "$MODEL_FILE" "$@"
EOF

chmod +x run.sh
print_success "Run script created (run.sh)"

# 11. Create project structure summary
print_status "Project structure created:"
echo "
📁 Project Structure:
├── 📁 include/           - Header files
├── 📁 src/              - Source files  
├── 📁 third_party/      - Dependencies (git submodules)
│   ├── 📁 whisper.cpp/  - Whisper C++ implementation
│   └── 📁 rtaudio/      - RtAudio library
├── 📁 build/            - Build artifacts
├── 📄 CMakeLists.txt    - Build configuration
├── 📄 setup.sh          - This setup script
├── 📄 run.sh            - Simple run script
├── 📄 ggml-base.en.bin  - Whisper model
└── 📄 README.md         - Documentation
"

print_success "🎉 Setup complete!"
echo
echo "🚀 Quick Start:"
echo "  1. ./run.sh                    # Start with default settings"
echo "  2. ./run.sh --help            # See all options"
echo "  3. ./run.sh --list-devices    # List audio devices"
echo
echo "🔧 Advanced:"
echo "  - Edit CMakeLists.txt to customize build"
echo "  - Check include/ and src/ for code structure"
echo "  - Download other models from: https://huggingface.co/ggerganov/whisper.cpp"
echo
print_success "Happy transcribing! 🎤✨"