#pragma once

#include <string>
#include <memory>
#include <vector>

// Forward declare llama types to avoid including llama.h in header
struct llama_model;
struct llama_context;
typedef int32_t llama_token;

/**
 * @brief LLM client for text summarization and chat using llama.cpp
 */
class LLMClient
{
public:
    /**
     * @brief Configuration for LLM client
     */
    struct Config
    {
        std::string modelPath;    ///< Path to GGUF model file
        int threads = 4;          ///< Number of threads for inference
        int contextSize = 32768;  ///< Context window size
        int maxTokens = 4096;     ///< Maximum tokens to generate
        float temperature = 0.7f; ///< Sampling temperature
        float topP = 0.9f;        ///< Top-p sampling
        bool verbose = false;     ///< Enable verbose logging
    };

    /**
     * @brief LLM response structure
     */
    struct Response
    {
        std::string text;       ///< Generated text
        int tokensGenerated;    ///< Number of tokens generated
        double inferenceTimeMs; ///< Inference time in milliseconds
        bool success;           ///< Whether generation was successful
        std::string error;      ///< Error message if failed
    };

    /**
     * @brief Constructor
     * @param config LLM configuration
     */
    explicit LLMClient(const Config &config);

    /**
     * @brief Destructor
     */
    ~LLMClient();

    /**
     * @brief Initialize the LLM (load model)
     * @return true on success, false on failure
     */
    bool initialize();

    /**
     * @brief Summarize a transcript
     * @param transcript The transcript text to summarize
     * @return LLM response with summary
     */
    Response summarizeTranscript(const std::string &transcript);

    /**
     * @brief Chat with context from transcripts
     * @param question User's question
     * @param context Relevant transcript context
     * @return LLM response
     */
    Response chatWithContext(const std::string &question, const std::string &context);

    /**
     * @brief Check if LLM is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

private:
    Config config_;
    llama_model *model_;     // Forward declared, defined in .cpp
    llama_context *context_; // Forward declared, defined in .cpp
    bool initialized_;

    /**
     * @brief Generate text using the model
     * @param prompt Input prompt
     * @param maxTokens Maximum tokens to generate
     * @return LLM response
     */
    Response generate(const std::string &prompt, int maxTokens = -1);

    /**
     * @brief Chat with system and user messages (Qwen format)
     * @param system_prompt System prompt for context
     * @param user_message User's message
     * @param maxTokens Maximum tokens to generate
     * @return LLM response
     */
    Response chat(const std::string &system_prompt, const std::string &user_message, int maxTokens = -1);

    /**
     * @brief Tokenize text
     * @param text Input text
     * @return Vector of token IDs
     */
    std::vector<llama_token> tokenize(const std::string &text);

    /**
     * @brief Detokenize tokens back to text
     * @param tokens Vector of token IDs
     * @return Detokenized text
     */
    std::string detokenize(const std::vector<llama_token> &tokens);
};