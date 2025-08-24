#include "LLamaServer.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

// Helper function to capture CURL response
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

LLamaServer::LLamaServer()
{
    // Constructor
}

LLamaServer::~LLamaServer()
{
    shutdown();
}

bool LLamaServer::initialize()
{
    // Initialize CURL globally
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK)
    {
        std::cerr << "Failed to initialize CURL: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    // TODO: Add any additional initialization logic here
    // For now, we assume the llama-server is already running externally
    return true;
}

void LLamaServer::shutdown()
{
    // Cleanup CURL
    curl_global_cleanup();
    std::cout << "LLamaServer shutdown" << std::endl;
}

std::string LLamaServer::generateResponse(const std::string &prompt)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string response_string;
    struct curl_slist *headers = nullptr;

    try
    {
        // Create JSON payload
        nlohmann::json request_body = {
            {"prompt", prompt},
            {"n_predict", 1024}};

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

        if (res != CURLE_OK)
        {
            std::string error_msg = "CURL error: ";
            error_msg += curl_easy_strerror(res);
            throw std::runtime_error(error_msg);
        }

        // Check HTTP response code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code != 200)
        {
            std::ostringstream oss;
            oss << "HTTP error: " << response_code;
            throw std::runtime_error(oss.str());
        }

        // Parse JSON response
        nlohmann::json json_response = nlohmann::json::parse(response_string);

        // Extract content from response
        if (json_response.contains("content"))
        {
            return json_response["content"].get<std::string>();
        }
        else
        {
            throw std::runtime_error("Response does not contain 'content' field");
        }
    }
    catch (const nlohmann::json::exception &e)
    {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::runtime_error("JSON error: " + std::string(e.what()));
    }
    catch (const std::exception &e)
    {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw;
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return "";
}
