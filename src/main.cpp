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

#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "AudioCapture.h"
#include "WhisperTranscriber.h"

#define USE_RTAUDIO 1

namespace
{
    // Global flag for graceful shutdown
    volatile std::sig_atomic_t g_shouldStop = 0;

    /**
     * @brief Signal handler for graceful shutdown
     */
    void signalHandler(int signal)
    {
        std::cout << "\n🛑 Received signal " << signal << ", shutting down gracefully..." << std::endl;
        g_shouldStop = 1;
    }

    /**
     * @brief Print application header
     */
    void printHeader()
    {
        std::cout << "🎤 Audio Transcriber v1.0.0" << std::endl;
        std::cout << "Real-time speech transcription using Whisper" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
    }

    /**
     * @brief Print usage information
     */
    void printUsage(const char *programName)
    {
        std::cout << "Usage: " << programName << " <model_path> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --device <id>      Audio input device ID (default: 0)" << std::endl;
        std::cout << "  --language <code>  Language code (en, es, fr, etc. or 'auto')" << std::endl;
        std::cout << "  --threads <num>    Number of threads for processing (default: 4)" << std::endl;
        std::cout << "  --list-devices     List available audio devices" << std::endl;
        std::cout << "  --help            Show this help message" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << programName << " ggml-base.en.bin" << std::endl;
        std::cout << "  " << programName << " ggml-small.en.bin --language auto" << std::endl;
        std::cout << "  " << programName << " ggml-base.en.bin --device 1 --threads 8" << std::endl;
        std::cout << std::endl;
        std::cout << "Download models from:" << std::endl;
        std::cout << "  https://huggingface.co/ggerganov/whisper.cpp/tree/main" << std::endl;
    }

    /**
     * @brief Get current timestamp as formatted string
     */
    std::string getCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    /**
     * @brief Parse command line arguments
     */
    struct Config
    {
        std::string modelPath;
        unsigned int deviceId = 1;
        std::string language = "auto";
        int threads = 4;
        bool listDevices = false;
        bool showHelp = false;
        bool valid = true;
        std::string error;
    };

    Config parseArguments(int argc, char *argv[])
    {
        Config config;

        if (argc < 2)
        {
            config.valid = false;
            config.error = "No model path specified";
            return config;
        }

        config.modelPath = argv[1];

        for (int i = 2; i < argc; i++)
        {
            std::string arg = argv[i];

            if (arg == "--help")
            {
                config.showHelp = true;
            }
            else if (arg == "--list-devices")
            {
                config.listDevices = true;
            }
            else if (arg == "--device" && i + 1 < argc)
            {
                config.deviceId = std::stoi(argv[++i]);
            }
            else if (arg == "--language" && i + 1 < argc)
            {
                config.language = argv[++i];
            }
            else if (arg == "--threads" && i + 1 < argc)
            {
                config.threads = std::stoi(argv[++i]);
            }
            else
            {
                config.valid = false;
                config.error = "Unknown argument: " + arg;
                return config;
            }
        }

        return config;
    }

    /**
     * @brief List available audio devices
     */
    void listAudioDevices()
    {
        AudioCapture capture;
        if (!capture.initialize())
        {
            std::cerr << "❌ Failed to initialize audio system" << std::endl;
            return;
        }

        auto devices = capture.getAvailableDevices();

        std::cout << "📱 Available Audio Input Devices:" << std::endl;
        std::cout << "──────────────────────────────────" << std::endl;

        for (size_t i = 0; i < devices.size(); i++)
        {
            std::cout << "  " << i << ": " << devices[i];
            if (i == 0)
            {
                std::cout << " (default)";
            }
            std::cout << std::endl;
        }

        if (devices.empty())
        {
            std::cout << "  No audio input devices found" << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    auto config = parseArguments(argc, argv);

    if (!config.valid)
    {
        std::cerr << "❌ Error: " << config.error << std::endl
                  << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    if (config.showHelp)
    {
        printHeader();
        printUsage(argv[0]);
        return 0;
    }

    if (config.listDevices)
    {
        printHeader();
        listAudioDevices();
        return 0;
    }

    // Print header
    printHeader();

    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try
    {
        // Initialize Whisper transcriber
        std::cout << "🤖 Loading Whisper model: " << config.modelPath << std::endl;

        WhisperTranscriber::Config whisperConfig;
        whisperConfig.modelPath = config.modelPath;
        whisperConfig.language = config.language;
        whisperConfig.threads = config.threads;

        WhisperTranscriber transcriber(whisperConfig);

        if (!transcriber.initialize())
        {
            std::cerr << "❌ Failed to initialize Whisper transcriber" << std::endl;
            std::cerr << "   Please check that the model file exists and is valid" << std::endl;
            return 1;
        }

        std::cout << "✅ Whisper model loaded successfully" << std::endl;

        // Initialize audio capture
        std::cout << "🎙️  Initializing audio capture..." << std::endl;

        AudioCapture::Config audioConfig;
        audioConfig.deviceId = config.deviceId;

        AudioCapture capture(audioConfig);

        capture.printAvailableDevices(); // Ensure devices are populated

        if (!capture.initialize())
        {
            std::cerr << "❌ Failed to initialize audio capture" << std::endl;
            std::cerr << "   Please check that your microphone is connected and accessible" << std::endl;
            return 1;
        }

        // List the device we're using
        auto devices = capture.getAvailableDevices();
        if (config.deviceId < devices.size())
        {
            std::cout << "🎧 Using audio device: " << devices[config.deviceId] << std::endl;
        }
        std::cout << "✅ Audio capture initialized" << std::endl;
        std::cout << std::endl;

        static std::string consolidatedText;

        // Set up real-time transcription callback
        transcriber.startRealTimeProcessing([](const WhisperTranscriber::Result &result)
                                            {
            if (!result.text.empty()) {
                consolidatedText += result.text + " ";
                // clear the console line
                system("clear");
                std::cout << consolidatedText << std::endl;
                // Optionally, you can print the result immediately
                // std::cout << "[" << getCurrentTimestamp() << "] " << result.text << std::endl;
            } });

        // Start audio capture with callback
        bool captureStarted = capture.start([&transcriber](const std::vector<float> &audioData, double timestamp)
                                            { transcriber.addAudioData(audioData, timestamp); });

        if (!captureStarted)
        {
            std::cerr << "❌ Failed to start audio capture" << std::endl;
            return 1;
        }

        std::cout << "🎤 Listening... (Press Ctrl+C to stop)" << std::endl;
        std::cout << "═══════════════════════════════════" << std::endl;

        // Main loop - wait for shutdown signal
        while (!g_shouldStop)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Cleanup
        std::cout << std::endl
                  << "🛑 Stopping..." << std::endl;

        capture.stop();
        transcriber.stopRealTimeProcessing();

        std::cout << "✅ Shutdown complete" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "❌ Unknown fatal error occurred" << std::endl;
        return 1;
    }

    return 0;
}