#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

class LLamaServer
{
public:
    LLamaServer();
    ~LLamaServer();

    bool initialize();
    void shutdown();

    std::string generateResponse(const std::string &prompt);

private:
    // Add any other necessary member variables
};