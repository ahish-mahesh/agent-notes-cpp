#include "LLMClient.h"
#include "LLamaServer.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

LLMClient::LLMClient(const Config &config)
    : config_(config), llamaServer_(nullptr), initialized_(false) {}

LLMClient::~LLMClient() {
  if (llamaServer_) {
    llamaServer_->shutdown();
    llamaServer_.reset();
  }
}

bool LLMClient::initialize() {
  if (initialized_)
    return true;

  // Check if model file exists
  std::ifstream modelFile(config_.modelPath);
  if (!modelFile.good()) {
    std::cerr << "❌ Model file not found: " << config_.modelPath << std::endl;
    return false;
  }

  // Initialize LLamaServer (network-based server)
  llamaServer_ = std::make_unique<LLamaServer>(config_.modelPath);
  if (!llamaServer_->initialize()) {
    std::cerr << "❌ Failed to initialize LLamaServer" << std::endl;
    llamaServer_.reset();
    return false;
  }

  initialized_ = true;
  std::cout << "✅ LLM client initialized with model: " << config_.modelPath
            << std::endl;
  return true;
}

LLMClient::Response
LLMClient::summarizeTranscript(const std::string &transcript) {
  if (!initialized_) {
    return {.success = false, .error = "LLM not initialized"};
  }

  // Use chat format optimized for small models with explicit stopping
  std::string system_prompt =
      "You are a helpful assistant that creates concise summaries of lecture "
      "transcripts. Always end your summary with a clear conclusion.";

  std::string user_message =
      "Summarize this university lecture transcript using this EXACT "
      "format:\n\n"
      "## Key Concepts and Definitions:\n"
      "[List the main concepts and their definitions here]\n\n"
      "## Important Formulas or Theories:\n"
      "[List any formulas, theories, or scientific principles mentioned]\n\n"
      "## Examples Given by the Professor:\n"
      "[List specific examples or case studies mentioned]\n\n"
      "## Potential Exam Topics:\n"
      "[List topics that would likely appear on an exam]\n\n"
      "Transcript:\n\n" +
      transcript +
      "\n\nUse the exact section headers shown above and organize your "
      "response accordingly." +
      "\n\nAfter providing the summary with the above mentioned format, end "
      "with 'Summary complete.'";

  return chat(system_prompt, user_message,
              4096); // Optimized tokens for longer summaries
}

LLMClient::Response LLMClient::chatWithContext(const std::string &question,
                                               const std::string &context) {
  if (!initialized_) {
    return {.success = false, .error = "LLM not initialized"};
  }

  // Use chat format for better context understanding
  std::string system_prompt = "You are a helpful assistant that answers "
                              "questions based on lecture content.";

  std::string user_message =
      "Context: " + context + "\n\nQuestion: " + question;

  return chat(system_prompt, user_message, config_.maxTokens);
}

bool LLMClient::isInitialized() const { return initialized_; }

LLMClient::Response LLMClient::generate(const std::string &prompt,
                                        int maxTokens) {
  auto start = std::chrono::high_resolution_clock::now();

  if (maxTokens <= 0)
    maxTokens = config_.maxTokens;

  if (!initialized_ || !llamaServer_) {
    return {.success = false, .error = "LLM not properly initialized"};
  }

  Response result;
  try {
    std::string text = llamaServer_->generateResponse(prompt);
    result.success = true;
    result.text = text;
    result.tokensGenerated = 0; // Not available from server response
    result.inferenceTimeMs = 0.0;
  } catch (const std::exception &e) {
    result.success = false;
    result.error = e.what();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  if (result.success && result.inferenceTimeMs == 0.0) {
    result.inferenceTimeMs = static_cast<double>(duration.count());
  }

  return result;
}

LLMClient::Response LLMClient::chat(const std::string &system_prompt,
                                    const std::string &user_message,
                                    int maxTokens) {
  // Compose a single prompt for the server
  std::string full_prompt = system_prompt + "\n\n" + user_message;
  return generate(full_prompt, maxTokens);
}