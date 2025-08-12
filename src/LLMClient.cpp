#include "LLMClient.h"
#include "LlamaBridge.h"

#include <iostream>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

LLMClient::LLMClient(const Config &config)
    : config_(config), model_(nullptr), context_(nullptr), initialized_(false)
{
}

LLMClient::~LLMClient()
{
    if (context_)
    {
        llama_bridge_free(reinterpret_cast<llama_bridge_context*>(context_));
        context_ = nullptr;
    }
    model_ = nullptr; // Not used with bridge API
}

bool LLMClient::initialize()
{
    if (initialized_)
        return true;

    // Check if model file exists
    std::ifstream modelFile(config_.modelPath);
    if (!modelFile.good())
    {
        std::cerr << "❌ Model file not found: " << config_.modelPath << std::endl;
        return false;
    }

    // Initialize llama bridge
    llama_bridge_params params = {};
    params.model_path = config_.modelPath.c_str();
    params.threads = config_.threads;
    params.context_size = config_.contextSize;
    params.max_tokens = config_.maxTokens;
    params.temperature = config_.temperature;
    params.top_p = config_.topP;
    params.verbose = config_.verbose;

    llama_bridge_context* bridge_ctx = llama_bridge_init(params);
    if (!bridge_ctx)
    {
        std::cerr << "❌ Failed to initialize LLM bridge" << std::endl;
        return false;
    }

    context_ = reinterpret_cast<llama_context*>(bridge_ctx);
    model_ = nullptr; // Not used with bridge API
    initialized_ = true;
    std::cout << "✅ LLM client initialized with model: " << config_.modelPath << std::endl;
    return true;
}

LLMClient::Response LLMClient::summarizeTranscript(const std::string &transcript)
{
    if (!initialized_)
    {
        return {.success = false, .error = "LLM not initialized"};
    }

    std::string prompt =
        "Summarize this university lecture transcript. Focus on:\n"
        "1. Key concepts and definitions\n"
        "2. Important formulas or theories\n"
        "3. Examples given by the professor\n"
        "4. Potential exam topics\n\n"
        "Transcript:\n" +
        transcript + "\n\n"
                     "Summary:";

    return generate(prompt, 512);
}

LLMClient::Response LLMClient::chatWithContext(const std::string &question, const std::string &context)
{
    if (!initialized_)
    {
        return {.success = false, .error = "LLM not initialized"};
    }

    std::string prompt =
        "Based on this lecture content, answer the following question:\n\n"
        "Context:\n" +
        context + "\n\n"
                  "Question: " +
        question + "\n\nAnswer:";

    return generate(prompt, config_.maxTokens);
}

bool LLMClient::isInitialized() const
{
    return initialized_;
}

LLMClient::Response LLMClient::generate(const std::string &prompt, int maxTokens)
{
    auto start = std::chrono::high_resolution_clock::now();

    if (maxTokens <= 0)
        maxTokens = config_.maxTokens;

    if (!initialized_ || !context_)
    {
        return {.success = false, .error = "LLM not properly initialized"};
    }

    // Use the bridge API for generation
    llama_bridge_context* bridge_ctx = reinterpret_cast<llama_bridge_context*>(context_);
    llama_bridge_result bridge_result = llama_bridge_generate(bridge_ctx, prompt.c_str(), maxTokens);

    Response result;
    result.success = bridge_result.success;
    
    if (bridge_result.success)
    {
        result.text = bridge_result.text ? std::string(bridge_result.text) : "";
        result.tokensGenerated = bridge_result.tokens_generated;
        result.inferenceTimeMs = bridge_result.inference_time_ms;
    }
    else
    {
        result.error = bridge_result.error_msg ? std::string(bridge_result.error_msg) : "Unknown error";
    }

    // Clean up bridge result
    llama_bridge_free_result(&bridge_result);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    if (result.success && result.inferenceTimeMs == 0.0)
    {
        result.inferenceTimeMs = static_cast<double>(duration.count());
    }

    return result;
}

std::vector<llama_token> LLMClient::tokenize(const std::string &text)
{
    if (!context_)
    {
        return {};
    }

    llama_bridge_context* bridge_ctx = reinterpret_cast<llama_bridge_context*>(context_);
    llama_bridge_tokens bridge_tokens = llama_bridge_tokenize(bridge_ctx, text.c_str());
    
    std::vector<llama_token> tokens;
    if (bridge_tokens.tokens && bridge_tokens.count > 0)
    {
        tokens.resize(bridge_tokens.count);
        for (int i = 0; i < bridge_tokens.count; i++)
        {
            tokens[i] = bridge_tokens.tokens[i];
        }
    }
    
    llama_bridge_free_tokens(&bridge_tokens);
    return tokens;
}

std::string LLMClient::detokenize(const std::vector<llama_token> &tokens)
{
    if (!context_ || tokens.empty())
    {
        return "";
    }

    llama_bridge_context* bridge_ctx = reinterpret_cast<llama_bridge_context*>(context_);
    llama_bridge_tokens bridge_tokens;
    bridge_tokens.count = tokens.size();
    bridge_tokens.tokens = const_cast<int*>(tokens.data());
    
    char* result_str = llama_bridge_detokenize(bridge_ctx, &bridge_tokens);
    std::string result = result_str ? std::string(result_str) : "";
    
    if (result_str)
    {
        free(result_str);
    }
    
    return result;
}