#!/bin/bash

# Simple run script for Audio Transcriber

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
