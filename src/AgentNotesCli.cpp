#include <iostream>
#include <sstream>

#include "AgentNotesCli.h"
#include "AudioCapture.h"
#include "DBHelper.h"
#include "LLMClient.h"
#include "WhisperTranscriber.h"

// Initialize static variables
volatile std::sig_atomic_t AgentNotesCli::g_shouldStop = 0;

AgentNotesCli::AgentNotesCli(Config cfg) : running(false), config(cfg) {}

void AgentNotesCli::initialize() {

  // Initialize the SQLite database
  std::cout << "📦 Initializing SQLite database..." << std::endl;
  dbHelper = std::make_unique<DBHelper>("transcriptions.db");
  std::cout << "✅ Database initialized successfully" << std::endl;

  // Initialize Whisper transcriber
  std::cout << "🤖 Loading Whisper model: " << config.modelPath << std::endl;

  WhisperTranscriber::Config whisperConfig;
  whisperConfig.modelPath = config.modelPath;
  whisperConfig.language = config.language;
  whisperConfig.threads = config.threads;

  whisperTranscriber = std::make_unique<WhisperTranscriber>(whisperConfig);

  if (!whisperTranscriber->initialize()) {
    std::cerr << "❌ Failed to initialize Whisper transcriber" << std::endl;
    std::cerr << "   Please check that the model file exists and is valid"
              << std::endl;
    return;
  }

  std::cout << "✅ Whisper model loaded successfully" << std::endl;

  // Initialize audio capture
  std::cout << "🎙️  Initializing audio capture..." << std::endl;

  AudioCapture::Config audioConfig;
  audioConfig.deviceId = config.deviceId;

  capture = std::make_unique<AudioCapture>(audioConfig);

  capture->printAvailableDevices(); // Ensure devices are populated

  if (!capture->initialize()) {
    std::cerr << "❌ Failed to initialize audio capture" << std::endl;
    std::cerr
        << "   Please check that your microphone is connected and accessible"
        << std::endl;
    return;
  }

  // List the device we're using
  auto devices = capture->getAvailableDevices();
  if (config.deviceId < devices.size()) {
    std::cout << "🎧 Using audio device: " << devices[config.deviceId]
              << std::endl;
  }
  std::cout << "✅ Audio capture initialized" << std::endl;
  std::cout << std::endl;

  // Initialize LLM Client
  LLMClient::Config llmConfig;
  llmConfig.modelPath = config.modelPath;
  llmConfig.threads = config.threads;
  llmConfig.contextSize = 32768;
  llmConfig.maxTokens = 32768;
  llmConfig.temperature = 0.7f;

  llmClient = std::make_unique<LLMClient>(llmConfig);

  if (llmClient->initialize()) {
    std::cout << "✅ LLM Client initialized successfully" << std::endl;
  } else {
    std::cerr << "❌ Failed to initialize LLM Client" << std::endl;
  }
}

AgentNotesCli::~AgentNotesCli() {
  whisperTranscriber.reset();
  llmClient.reset();
  capture.reset();
  dbHelper.reset();
  printGoodbyeMessage();
}

void AgentNotesCli::run() {

  if (!config.valid) {
    std::cerr << "❌ Config is not valid: " << config.error << std::endl;
    return;
  }

  printWelcomeMessage();

  running = true;
  while (running) {
    std::string input = getUserInput();
    processUserInput(input);
  }
}

void AgentNotesCli::printWelcomeMessage() {
  // Clear terminal output
  // std::cout << "\033[2J\033[1;1H"; // ANSI escape codes to clear the screen
  std::cout << "Welcome to Agent Notes CLI!" << std::endl;
  std::cout << "Type 'help' to see available commands." << std::endl;
}

void AgentNotesCli::printGoodbyeMessage() {
  std::cout << "Goodbye! Thank you for using Agent Notes CLI." << std::endl;
}

void AgentNotesCli::printHelp() {
  std::cout << "Available commands:" << std::endl;
  std::cout << "  help               Show this help message" << std::endl;
  std::cout << "  start              Start transcription" << std::endl;
  std::cout << "  newchat            Start a new chat" << std::endl;
  std::cout << "  listchats          List all chats" << std::endl;
  std::cout << "  deletechat <id>    Delete chat by ID" << std::endl;
  std::cout << "  listnotes          List all notes" << std::endl;
  std::cout << "  deletenote <id>    Delete note by ID" << std::endl;
  std::cout << "  exit               Exit the application" << std::endl;
}

std::string AgentNotesCli::getUserInput() {
  std::string input;
  std::cout << "> ";
  std::getline(std::cin, input);
  return input;
}

Config AgentNotesCli::parseArguments(int argc, char *argv[]) {
  Config config;

  if (argc < 2) {
    config.valid = false;
    config.error = "No model path specified";
    return config;
  }

  config.modelPath = argv[1];

  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "--help") {
      config.showHelp = true;
    } else if (arg == "--list-devices") {
      config.listDevices = true;
    } else if (arg == "--device" && i + 1 < argc) {
      config.deviceId = std::stoi(argv[++i]);
    } else if (arg == "--language" && i + 1 < argc) {
      config.language = argv[++i];
    } else if (arg == "--threads" && i + 1 < argc) {
      config.threads = std::stoi(argv[++i]);
    } else {
      config.valid = false;
      config.error = "Unknown argument: " + arg;
      return config;
    }
  }

  return config;
}

void AgentNotesCli::processUserInput(const std::string &input) {
  std::istringstream iss(input);
  std::string command;
  iss >> command;

  if (command == "help") {
    printHelp();
  } else if (command == "start") {
    startTranscription();
  } else if (command == "newchat") {
    newChat();
  } else if (command == "listchats") {
    listChats();
  } else if (command == "deletechat") {
    int id;
    if (iss >> id) {
      deleteChat(id);
    } else {
      std::cout << "Invalid command format. Usage: deletechat <id>"
                << std::endl;
    }
  } else if (command == "create") {
    std::string content;
    std::getline(iss, content);
    if (!content.empty()) {
      createNote(content.substr(1)); // Remove leading space
    } else {
      std::cout << "Note content cannot be empty." << std::endl;
    }
  } else if (command == "listnotes") {
    listNotes();
  } else if (command == "deletenote") {
    int id;
    if (iss >> id) {
      deleteNote(id);
    } else {
      std::cout << "Invalid command format. Usage: deletenote <id>"
                << std::endl;
    }
  } else if (command == "exit") {
    running = false;
  } else {
    std::cout << "Unknown command: " << command
              << ". Type 'help' to see available commands." << std::endl;
  }
}

void AgentNotesCli::signalHandler(int signal) {
  std::cout << "\n🛑 Received signal " << signal
            << ", shutting down gracefully..." << std::endl;
  g_shouldStop = 1;
}

#pragma region Transcription

void AgentNotesCli::startTranscription() {
  std::cout << "Starting transcription... (Press Ctrl+C to stop)" << std::endl;

  // Set up signal handlers for graceful shutdown
  std::signal(SIGINT, AgentNotesCli::signalHandler);
  std::signal(SIGTERM, AgentNotesCli::signalHandler);

  try {

    static std::string consolidatedText;

    // Set up real-time transcription callback
    whisperTranscriber->startRealTimeProcessing(
        [](const WhisperTranscriber::Result &result) {
          if (!result.text.empty()) {
            consolidatedText += result.text + " ";
            // clear the console line
            system("clear");
            std::cout << consolidatedText << std::endl;
            // Optionally, you can print the result immediately
            // std::cout << "[" << getCurrentTimestamp() << "] " << result.text
            // << std::endl;
          }
        });

    // Start audio capture with callback
    bool captureStarted = capture->start(
        [this](const std::vector<float> &audioData, double timestamp) {
          this->whisperTranscriber->addAudioData(audioData, timestamp);
        });

    if (!captureStarted) {
      std::cerr << "❌ Failed to start audio capture" << std::endl;
      return;
    }

    std::cout << "🎤 Listening... (Press Ctrl+C to stop)" << std::endl;
    std::cout << "═══════════════════════════════════" << std::endl;

    // Main loop - wait for shutdown signal
    while (!g_shouldStop) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    std::cout << std::endl << "🛑 Stopping..." << std::endl;

    capture->stop();
    whisperTranscriber->stopRealTimeProcessing();

    // Stop audio capture and transcription and save the final text to the DB
    std::cout << "\n📝 Saving final transcription to database..." << std::endl;

    // clone the consolidated text
    const std::string finalTranscription = consolidatedText;

    // Ask the user whether they want to save the transcription to the DB
    std::string choice;
    std::cout
        << "Do you want to save the transcription to the database? (y/n): ";
    std::cin >> choice;

    if (choice == "y" || choice == "Y") {
      if (!dbHelper->SaveTranscriptionResult(finalTranscription)) {
        std::cerr << "❌ Failed to save transcription to database" << std::endl;
      } else {
        std::cout << "✅ Transcription saved to database successfully"
                  << std::endl;
      }
    }

    // Summarize the transcription
    summarizeTranscript(finalTranscription);

    std::cout << "✅ Shutdown complete" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "❌ Fatal error: " << e.what() << std::endl;
    return;
  } catch (...) {
    std::cerr << "❌ Unknown fatal error occurred" << std::endl;
    return;
  }

  return;
}

void AgentNotesCli::summarizeTranscript(std::string finalTranscription) {
  // Initialize LLM client
  std::cout << "🤖 Initializing LLM for summarization..." << std::endl;

  if (llmClient->isInitialized()) {
    std::cout << "🧠 Generating summary..." << std::endl;

    auto summaryResponse = llmClient->summarizeTranscript(finalTranscription);

    if (summaryResponse.success) {
      std::cout << "\n📝 SUMMARY:" << std::endl;
      std::cout << "═══════════" << std::endl;
      std::cout << summaryResponse.text << std::endl;
      std::cout << "\n⚡ Generated " << summaryResponse.tokensGenerated
                << " tokens in " << summaryResponse.inferenceTimeMs << "ms"
                << std::endl;

      // TODO: Save summary to database
    } else {
      std::cerr << "❌ Failed to generate summary: " << summaryResponse.error
                << std::endl;
    }
  } else {
    std::cerr << "❌ Failed to initialize LLM client" << std::endl;
  }
}

#pragma endregion Transcription

#pragma region Chat

void AgentNotesCli::newChat() {
  std::cout << "Starting a new chat... (not implemented)" << std::endl;
  // Placeholder for starting a new chat logic
}

void AgentNotesCli::listChats() {
  std::cout << "Listing all chats... (not implemented)" << std::endl;
  // Placeholder for listing all chats logic
}

void AgentNotesCli::deleteChat(int id) {
  std::cout << "Deleting chat with ID " << id << "... (not implemented)"
            << std::endl;
  // Placeholder for deleting a chat logic
}

#pragma endregion Chat

#pragma region Notes CRUD

void AgentNotesCli::createNote(const std::string &content) {
  notes.push_back(content);
  std::cout << "Note created." << std::endl;
}

void AgentNotesCli::listNotes() {
  if (notes.empty()) {
    std::cout << "No notes available." << std::endl;
    return;
  }

  std::cout << "Listing all notes:" << std::endl;
  for (size_t i = 0; i < notes.size(); ++i) {
    std::cout << "  [" << i << "] " << notes[i] << std::endl;
  }
}

void AgentNotesCli::deleteNote(int id) {
  if (id < 0 || static_cast<size_t>(id) >= notes.size()) {
    std::cout << "Invalid note ID." << std::endl;
    return;
  }

  notes.erase(notes.begin() + id);
  std::cout << "Note deleted." << std::endl;
}

#pragma endregion Notes CRUD
