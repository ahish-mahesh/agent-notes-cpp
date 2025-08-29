/**
 * @file main.cpp
 * @brief Audio Transcriber - Real-time speech-to-text using Whisper
 *
 * This application captures audio from your microphone and transcribes it
 * in real-time using OpenAI's Whisper model.
 *
 * Usage:
 *   ./audio-transcriber <model_path> [options]
 *
 * Example:
 *   ./audio-transcriber ggml-base.en.bin
 *   ./audio-transcriber ggml-base.en.bin --device 1 --language en
 */

#include <AgentNotesCli.h>
#include <csignal>

int main(int argc, char *argv[]) {
  AgentNotesCli cli(AgentNotesCli::parseArguments(argc, argv));
  cli.initialize();
  cli.run();
}