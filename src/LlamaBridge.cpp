#include "LlamaBridge.h"

// This file can include llama.h because it's in the llama_wrapper library
#include "llama.h"

#include <string>
#include <memory>
#include <cstring>
#include <iostream>
#include <vector>
#include <chrono>

// Internal implementation struct (can use llama/ggml types here)
struct llama_bridge_context
{
    struct llama_model *model;
    struct llama_context *ctx;
    struct llama_sampler *sampler;
    llama_bridge_params params;

    llama_bridge_context() : model(nullptr), ctx(nullptr), sampler(nullptr) {}
};

// Helper function to allocate and copy string
static char *allocate_string(const std::string &str)
{
    if (str.empty())
        return nullptr;
    char *result = (char *)malloc(str.length() + 1);
    if (result)
    {
        strcpy(result, str.c_str());
    }
    return result;
}

llama_bridge_context *llama_bridge_init(llama_bridge_params params)
{
    auto *bridge_ctx = new llama_bridge_context();
    bridge_ctx->params = params;

    // Initialize llama backend
    llama_backend_init();

    // Load model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 999; // Use CPU for compatibility
    model_params.use_mmap = true;
    model_params.use_mlock = true;

    bridge_ctx->model = llama_model_load_from_file(params.model_path, model_params);
    if (!bridge_ctx->model)
    {
        llama_backend_free();
        delete bridge_ctx;
        return nullptr;
    }

    // Create context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = params.context_size;
    // ctx_params.n_threads = params.threads;
    // ctx_params.n_threads_batch = params.threads;
    ctx_params.n_threads = std::min(params.threads, 8); // M1 optimization
    ctx_params.n_threads_batch = ctx_params.n_threads;
    ctx_params.flash_attn = true; // Enable flash attention if available

    bridge_ctx->ctx = llama_init_from_model(bridge_ctx->model, ctx_params);
    if (!bridge_ctx->ctx)
    {
        llama_model_free(bridge_ctx->model);
        llama_backend_free();
        delete bridge_ctx;
        return nullptr;
    }

    // Initialize sampler chain
    auto sparams = llama_sampler_chain_default_params();
    bridge_ctx->sampler = llama_sampler_chain_init(sparams);
    // Filters first
    if (params.top_p > 0.0f && params.top_p <= 1.0f)
    {
        llama_sampler_chain_add(bridge_ctx->sampler, llama_sampler_init_top_p(params.top_p, 1));
    }
    // Chooser: temperature (>0) or greedy (when temp==0)
    if (params.temperature > 0.0f)
    {
        llama_sampler_chain_add(bridge_ctx->sampler, llama_sampler_init_temp(params.temperature));

        // CRITICAL: Add a final chooser that actually selects a token
        // Use distribution sampling for non-zero temperature
        llama_sampler_chain_add(bridge_ctx->sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    }
    else
    {
        llama_sampler_chain_add(bridge_ctx->sampler, llama_sampler_init_greedy());
    }

    return bridge_ctx;
}

void llama_bridge_free(llama_bridge_context *ctx)
{
    if (!ctx)
        return;

    if (ctx->sampler)
    {
        llama_sampler_free(ctx->sampler);
    }
    if (ctx->ctx)
    {
        llama_free(ctx->ctx);
    }
    if (ctx->model)
    {
        llama_model_free(ctx->model);
    }
    llama_backend_free();
    delete ctx;
}

llama_bridge_result llama_bridge_generate(
    llama_bridge_context *ctx,
    const char *prompt,
    int max_tokens)
{

    llama_bridge_result result = {};
    auto start = std::chrono::high_resolution_clock::now();

    if (!ctx || !ctx->ctx || !ctx->model || !prompt)
    {
        result.success = false;
        result.error_msg = allocate_string("Invalid parameters");
        return result;
    }

    if (max_tokens <= 0)
    {
        max_tokens = ctx->params.max_tokens;
    }

    // Tokenize the prompt
    std::vector<llama_token> tokens(strlen(prompt) + 32);
    const struct llama_vocab *vocab = llama_model_get_vocab(ctx->model);
    int n_tokens = llama_tokenize(vocab, prompt, strlen(prompt), tokens.data(), tokens.size(), true, false);

    if (n_tokens < 0)
    {
        result.success = false;
        result.error_msg = allocate_string("Failed to tokenize prompt");
        return result;
    }
    tokens.resize(n_tokens);

    // Clear the KV cache
    llama_memory_clear(llama_get_memory(ctx->ctx), true);

    // Evaluate the prompt tokens
    struct llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    // Request logits for the last token in the prompt evaluation
    if (batch.n_tokens > 0 && batch.logits)
    {
        batch.logits[batch.n_tokens - 1] = 1;
    }
    // TODO: Debug stopped here. Take a look into why the llama_decode is failing
    // Also refer to the examples on how others have implemented it.
    if (llama_decode(ctx->ctx, batch) != 0)
    {
        result.success = false;
        result.error_msg = allocate_string("Failed to evaluate prompt");
        return result;
    }
    int n_pos = tokens.size();

    // Reset sampler state and accept prompt tokens so penalties work properly
    if (ctx->sampler)
    {
        llama_sampler_reset(ctx->sampler);
        for (auto t : tokens)
        {
            llama_sampler_accept(ctx->sampler, t);
        }
    }

    // Generate tokens
    std::string generated_text;
    int tokens_generated = 0;

    for (int i = 0; i < max_tokens; ++i)
    {
        // Use convenience API which applies chain and accepts the sampled token
        llama_token next_token = llama_sampler_sample(ctx->sampler, ctx->ctx, -1);

        // Check for end of text
        const struct llama_vocab *vocab = llama_model_get_vocab(ctx->model);
        if (llama_vocab_is_eog(vocab, next_token))
        {
            break;
        }

        // Convert token to text
        char token_str[256];
        int n = llama_token_to_piece(vocab, next_token, token_str, sizeof(token_str), 0, false);
        if (n < 0)
        {
            result.success = false;
            result.error_msg = allocate_string("Failed to convert token to text");
            return result;
        }

        // Print the string as and when it's generated
        // std::cout << token_str << " ";

        generated_text.append(token_str, n);
        tokens_generated++;

        // Evaluate the new token and request logits for it
        struct llama_batch next_batch = llama_batch_get_one(&next_token, 1);
        if (next_batch.n_tokens > 0 && next_batch.logits)
        {
            next_batch.logits[next_batch.n_tokens - 1] = 1;
        }
        if (llama_decode(ctx->ctx, next_batch) != 0)
        {
            result.success = false;
            result.error_msg = allocate_string("Failed to evaluate generated token");
            return result;
        }
        n_pos++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    result.success = true;
    result.text = allocate_string(generated_text);
    result.tokens_generated = tokens_generated;
    result.inference_time_ms = static_cast<double>(duration.count());

    return result;
}

llama_bridge_result llama_bridge_chat(
    llama_bridge_context *ctx,
    const char *system_prompt,
    const char *user_message,
    int max_tokens)
{

    // Construct a chat-formatted prompt using Qwen2.5 format
    std::string full_prompt;
    if (system_prompt && strlen(system_prompt) > 0)
    {
        full_prompt = std::string("<|im_start|>system\n") + system_prompt + 
                     "<|im_end|>\n<|im_start|>user\n" + user_message + 
                     "<|im_end|>\n<|im_start|>assistant\n";
    }
    else
    {
        full_prompt = std::string("<|im_start|>user\n") + user_message + 
                     "<|im_end|>\n<|im_start|>assistant\n";
    }

    return llama_bridge_generate(ctx, full_prompt.c_str(), max_tokens);
}

void llama_bridge_free_result(llama_bridge_result *result)
{
    if (!result)
        return;

    if (result->text)
    {
        free(result->text);
        result->text = nullptr;
    }
    if (result->error_msg)
    {
        free(result->error_msg);
        result->error_msg = nullptr;
    }
}

llama_bridge_tokens llama_bridge_tokenize(llama_bridge_context *ctx, const char *text)
{
    llama_bridge_tokens result = {};

    if (!ctx || !ctx->model || !text)
    {
        return result;
    }

    std::vector<llama_token> tokens(strlen(text) + 32);
    const struct llama_vocab *vocab = llama_model_get_vocab(ctx->model);
    int n_tokens = llama_tokenize(vocab, text, strlen(text), tokens.data(), tokens.size(), true, false);

    if (n_tokens > 0)
    {
        result.tokens = (int *)malloc(n_tokens * sizeof(int));
        if (result.tokens)
        {
            for (int i = 0; i < n_tokens; i++)
            {
                result.tokens[i] = tokens[i];
            }
            result.count = n_tokens;
        }
    }

    return result;
}

char *llama_bridge_detokenize(llama_bridge_context *ctx, const llama_bridge_tokens *tokens)
{
    if (!ctx || !ctx->model || !tokens || !tokens->tokens)
    {
        return nullptr;
    }

    std::string result;
    for (int i = 0; i < tokens->count; i++)
    {
        char token_str[256];
        const struct llama_vocab *vocab = llama_model_get_vocab(ctx->model);
        int n = llama_token_to_piece(vocab, tokens->tokens[i], token_str, sizeof(token_str), 0, false);
        if (n > 0)
        {
            result.append(token_str, n);
        }
    }

    return allocate_string(result);
}

void llama_bridge_free_tokens(llama_bridge_tokens *tokens)
{
    if (!tokens)
        return;

    if (tokens->tokens)
    {
        free(tokens->tokens);
        tokens->tokens = nullptr;
    }
    tokens->count = 0;
}

int llama_bridge_get_context_size(llama_bridge_context *ctx)
{
    if (!ctx || !ctx->ctx)
        return 0;
    return llama_n_ctx(ctx->ctx);
}

int llama_bridge_get_vocab_size(llama_bridge_context *ctx)
{
    if (!ctx || !ctx->model)
        return 0;
    const struct llama_vocab *vocab = llama_model_get_vocab(ctx->model);
    return llama_vocab_n_tokens(vocab);
}