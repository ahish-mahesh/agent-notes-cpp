#include "WhisperBridge.h"

// This file can include whisper.h because it's in the whisper_wrapper library
#include "whisper.h"

#include <string>
#include <memory>
#include <cstring>
#include <iostream>

// Internal implementation struct (can use whisper/ggml types here)
struct whisper_bridge_context {
    struct whisper_context* ctx;
    whisper_bridge_params params;
    whisper_bridge_callback callback;
    void* user_data;
    bool streaming;
    
    whisper_bridge_context() : ctx(nullptr), callback(nullptr), user_data(nullptr), streaming(false) {}
};

// Helper function to allocate and copy string
static char* allocate_string(const std::string& str) {
    if (str.empty()) return nullptr;
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

whisper_bridge_context* whisper_bridge_init(whisper_bridge_params params) {
    auto* bridge_ctx = new whisper_bridge_context();
    bridge_ctx->params = params;
    
    // Initialize whisper
    struct whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = params.use_gpu;
    
    bridge_ctx->ctx = whisper_init_from_file_with_params(params.model_path, cparams);
    if (!bridge_ctx->ctx) {
        delete bridge_ctx;
        return nullptr;
    }
    
    return bridge_ctx;
}

void whisper_bridge_free(whisper_bridge_context* ctx) {
    if (!ctx) return;
    
    if (ctx->ctx) {
        whisper_free(ctx->ctx);
    }
    delete ctx;
}

whisper_bridge_result whisper_bridge_transcribe_audio(
    whisper_bridge_context* ctx,
    const float* audio_data,
    int audio_len,
    int sample_rate) {
    
    whisper_bridge_result result = {};
    
    if (!ctx || !ctx->ctx || !audio_data) {
        result.success = false;
        result.error_msg = allocate_string("Invalid parameters");
        return result;
    }
    
    // Set up whisper parameters
    struct whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.language = ctx->params.language;
    wparams.n_threads = ctx->params.threads;
    wparams.translate = false;
    wparams.print_progress = false;
    wparams.print_timestamps = false;
    
    // Run transcription
    int ret = whisper_full(ctx->ctx, wparams, audio_data, audio_len);
    if (ret != 0) {
        result.success = false;
        result.error_msg = allocate_string("Transcription failed");
        return result;
    }
    
    // Extract results
    std::string full_text;
    int n_segments = whisper_full_n_segments(ctx->ctx);
    
    for (int i = 0; i < n_segments; ++i) {
        const char* text = whisper_full_get_segment_text(ctx->ctx, i);
        if (text) {
            full_text += text;
        }
    }
    
    result.success = true;
    result.text = allocate_string(full_text);
    result.confidence = 0.9f; // Placeholder - whisper doesn't provide confidence scores
    result.start_time_ms = n_segments > 0 ? whisper_full_get_segment_t0(ctx->ctx, 0) * 10 : 0;
    result.end_time_ms = n_segments > 0 ? whisper_full_get_segment_t1(ctx->ctx, n_segments - 1) * 10 : 0;
    
    return result;
}

void whisper_bridge_free_result(whisper_bridge_result* result) {
    if (!result) return;
    
    if (result->text) {
        free(result->text);
        result->text = nullptr;
    }
    if (result->error_msg) {
        free(result->error_msg);
        result->error_msg = nullptr;
    }
}

bool whisper_bridge_start_stream(
    whisper_bridge_context* ctx, 
    whisper_bridge_callback callback,
    void* user_data) {
    
    if (!ctx || !callback) return false;
    
    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->streaming = true;
    
    return true;
}

void whisper_bridge_add_audio(
    whisper_bridge_context* ctx,
    const float* audio_data,
    int audio_len,
    double timestamp) {
    
    if (!ctx || !ctx->streaming || !audio_data) return;
    
    // For now, process each audio chunk immediately
    // In a real implementation, you'd buffer audio and process in chunks
    whisper_bridge_result result = whisper_bridge_transcribe_audio(ctx, audio_data, audio_len, 16000);
    
    if (ctx->callback && result.success && result.text && strlen(result.text) > 0) {
        ctx->callback(&result, ctx->user_data);
    }
    
    whisper_bridge_free_result(&result);
}

void whisper_bridge_stop_stream(whisper_bridge_context* ctx) {
    if (!ctx) return;
    
    ctx->streaming = false;
    ctx->callback = nullptr;
    ctx->user_data = nullptr;
}