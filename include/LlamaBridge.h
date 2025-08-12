#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Forward declare opaque handle types (no ggml exposure)
typedef struct llama_bridge_context llama_bridge_context;

// Configuration structure (plain C types only)
typedef struct {
    const char* model_path;
    int threads;
    int context_size;
    int max_tokens;
    float temperature;
    float top_p;
    bool verbose;
} llama_bridge_params;

// Result structure (plain C types only)
typedef struct {
    char* text;                 // Allocated string - caller must free
    int tokens_generated;
    double inference_time_ms;
    bool success;
    char* error_msg;           // Allocated string - caller must free on error
} llama_bridge_result;

// Token structure for advanced usage
typedef struct {
    int* tokens;               // Allocated array - caller must free
    int count;
} llama_bridge_tokens;

// API Functions
llama_bridge_context* llama_bridge_init(llama_bridge_params params);
void llama_bridge_free(llama_bridge_context* ctx);

llama_bridge_result llama_bridge_generate(
    llama_bridge_context* ctx,
    const char* prompt,
    int max_tokens
);

llama_bridge_result llama_bridge_chat(
    llama_bridge_context* ctx,
    const char* system_prompt,
    const char* user_message,
    int max_tokens
);

void llama_bridge_free_result(llama_bridge_result* result);

// Advanced tokenization functions
llama_bridge_tokens llama_bridge_tokenize(llama_bridge_context* ctx, const char* text);
char* llama_bridge_detokenize(llama_bridge_context* ctx, const llama_bridge_tokens* tokens);
void llama_bridge_free_tokens(llama_bridge_tokens* tokens);

// Utility functions
int llama_bridge_get_context_size(llama_bridge_context* ctx);
int llama_bridge_get_vocab_size(llama_bridge_context* ctx);

#ifdef __cplusplus
}
#endif