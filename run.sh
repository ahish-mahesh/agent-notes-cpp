#!/bin/bash

# Simple run script for Audio Transcriber
#
# Defaults (can be overridden by environment variables):
#  LLAMA_MODEL - path to the model used by llama-server (GGUF)
#  ASR_MODEL   - path to the whisper/ASR model used by the audio-transcriber

LLAMA_MODEL="${LLAMA_MODEL:-models/qwen2.5-0.5b-instruct-q4_k_m.gguf}"
ASR_MODEL="${ASR_MODEL:-models/ggml-base.en.bin}"

# Start llama-server in background if model exists
LLAMA_PID=""
if [ -f "$LLAMA_MODEL" ]; then
    echo "Starting llama-server with model: $LLAMA_MODEL"
    llama-server --model "$LLAMA_MODEL" --port 8081 
    LLAMA_PID=$!
    echo "llama-server started (pid=$LLAMA_PID)"
else
    echo "[WARNING] llama-server model not found: $LLAMA_MODEL"
    echo "If you want the server running, set LLAMA_MODEL or run ./setup.sh to download the model."
fi

cleanup() {
    if [ -n "$LLAMA_PID" ]; then
        echo "Stopping llama-server (pid=$LLAMA_PID)"
        kill "$LLAMA_PID" 2>/dev/null || true
        wait "$LLAMA_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT

if [ ! -f "build/audio-transcriber" ]; then
    echo "❌ Binary not found. Please run ./setup.sh first"
    exit 1
fi

if [ ! -f "$ASR_MODEL" ]; then
    echo "❌ ASR model file not found: $ASR_MODEL"
    echo "Please download a model or run ./setup.sh"
    exit 1
fi

echo "🎤 Starting Audio Transcriber..."
echo "Press Ctrl+C to stop"
echo

./build/audio-transcriber "$ASR_MODEL" "$@"
