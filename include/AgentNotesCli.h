#pragma once

#include <AudioCapture.h>
#include <DBHelper.h>
#include <LLMClient.h>
#include <WhisperTranscriber.h>
#include <csignal>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Parse command line arguments
 */
struct Config {
  std::string modelPath;
  unsigned int deviceId = 1;
  std::string language = "auto";
  int threads = 4;
  bool listDevices = false;
  bool showHelp = false;
  bool valid = true;
  std::string error;
};

/**
 * @class AgentNotesCli
 * @brief A command-line interface for managing agent notes. A POC before
 writing the backend server.

 */
class AgentNotesCli {
public:
  AgentNotesCli(Config cfg);
  ~AgentNotesCli();

  void initialize();

  // NOTE: This method should be called before run()
  static Config parseArguments(int argc, char *argv[]);
  void run();

private:
  void printWelcomeMessage();
  void printGoodbyeMessage();
  std::string getUserInput();
  void processUserInput(const std::string &input);
  void printHelp();

  // Transcription methods
  void startTranscription();
  void summarizeTranscript(std::string finalTranscription);

  // Chat methods
  void newChat();
  void listChats();
  void deleteChat(int id);

  // CRUD operations of Notes
  void createNote(const std::string &content);
  void listNotes();
  void deleteNote(int id);

  static void signalHandler(int signal);

  std::vector<std::string> notes;
  bool running;
  static volatile std::sig_atomic_t g_shouldStop;
  Config config;
  std::unique_ptr<LLMClient> llmClient;
  std::unique_ptr<WhisperTranscriber> whisperTranscriber;
  std::unique_ptr<DBHelper> dbHelper;
  std::unique_ptr<AudioCapture> capture;
};