#include "LLamaServer.h"
#include <chrono>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

// Helper function to capture CURL response
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

LLamaServer::LLamaServer(const std::string &modelPath) : _modelPath(modelPath) {
  // Constructor
}

LLamaServer::~LLamaServer() { shutdown(); }

bool LLamaServer::initialize() {
  // Initialize CURL globally
  CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
  if (res != CURLE_OK) {
    std::cerr << "Failed to initialize CURL: " << curl_easy_strerror(res)
              << std::endl;
    return false;
  }

  // Start the llama-server as a background process (fork + exec)
  if (serverRunning_.load()) {
    std::cerr << "llama-server already running (pid=" << serverPid_ << ")"
              << std::endl;
    return true;
  }

  pid_t pid = fork();
  if (pid < 0) {
    std::cerr << "Failed to fork for llama-server" << std::endl;
    return false;
  }

  if (pid == 0) {
    // Child process: replace with llama-server
    // Build argv
    std::vector<char *> args;
    args.push_back(const_cast<char *>("llama-server"));
    args.push_back(const_cast<char *>("--model"));
    args.push_back(const_cast<char *>(_modelPath.c_str()));
    args.push_back(nullptr);

    // Redirect child's stdio to /dev/null so it doesn't block the parent
    FILE *devnull = fopen("/dev/null", "w+");
    if (devnull) {
      dup2(fileno(devnull), STDOUT_FILENO);
      dup2(fileno(devnull), STDERR_FILENO);
      // keep stdin as is or redirect if desired
    }

    // Execute
    execvp("llama-server", args.data());

    // If execvp returns, it's an error
    std::cerr << "Failed to exec llama-server" << std::endl;
    _exit(EXIT_FAILURE);
  }

  // Parent process: store pid and mark running
  serverPid_ = pid;
  serverRunning_.store(true);

  // Optionally wait a short time for the server to initialize
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  return true;
}

void LLamaServer::shutdown() {
  // Terminate the background server if it was started
  if (serverRunning_.load() && serverPid_ > 0) {
    std::cout << "Shutting down llama-server (pid=" << serverPid_ << ")"
              << std::endl;
    // Ask process to terminate gracefully
    kill(serverPid_, SIGTERM);

    // Wait up to 5 seconds for the process to exit
    int status = 0;
    for (int i = 0; i < 50; ++i) {
      pid_t w = waitpid(serverPid_, &status, WNOHANG);
      if (w == serverPid_)
        break; // exited
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // If still running, force kill
    pid_t w = waitpid(serverPid_, &status, WNOHANG);
    if (w == 0) {
      std::cerr << "llama-server did not exit, sending SIGKILL" << std::endl;
      kill(serverPid_, SIGKILL);
      waitpid(serverPid_, &status, 0);
    }

    serverRunning_.store(false);
    serverPid_ = -1;
  }

  // Cleanup CURL
  curl_global_cleanup();
  std::cout << "LLamaServer shutdown" << std::endl;
}

std::string LLamaServer::generateResponse(const std::string &prompt) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    throw std::runtime_error("Failed to initialize CURL");
  }

  std::string response_string;
  struct curl_slist *headers = nullptr;

  try {
    // Create JSON payload
    nlohmann::json request_body = {{"prompt", prompt}, {"n_predict", 1024}};

    std::string json_string = request_body.dump();

    // Set headers
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Configure CURL
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8081/completion");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 300 second timeout

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      std::string error_msg = "CURL error: ";
      error_msg += curl_easy_strerror(res);
      throw std::runtime_error(error_msg);
    }

    // Check HTTP response code
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (response_code != 200) {
      std::ostringstream oss;
      oss << "HTTP error: " << response_code;
      throw std::runtime_error(oss.str());
    }

    // Parse JSON response
    nlohmann::json json_response = nlohmann::json::parse(response_string);

    // Extract content from response
    if (json_response.contains("content")) {
      return json_response["content"].get<std::string>();
    } else {
      throw std::runtime_error("Response does not contain 'content' field");
    }
  } catch (const nlohmann::json::exception &e) {
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    throw std::runtime_error("JSON error: " + std::string(e.what()));
  } catch (const std::exception &e) {
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    throw;
  }

  // Cleanup
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return "";
}
