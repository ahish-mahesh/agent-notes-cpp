#pragma once

#include <atomic>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>

// pid_t is POSIX; include for declaration
#include <sys/types.h>

class LLamaServer {
public:
  LLamaServer(const std::string &modelPath);
  ~LLamaServer();

  bool initialize();
  void shutdown();

  std::string generateResponse(const std::string &prompt);

private:
  std::string _modelPath;
  // PID of the background llama-server process (or -1 if not running)
  pid_t serverPid_ = -1;
  std::atomic<bool> serverRunning_{false};
};