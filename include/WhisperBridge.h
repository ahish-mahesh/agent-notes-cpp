#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Forward declare opaque handle types (no ggml exposure)
    typedef struct whisper_bridge_context whisper_bridge_context;

    // Configuration structure (plain C types only)
    typedef struct
    {
        const char *model_path;
        const char *language;
        int threads;
        int max_len_ms;
        float vad_threshold;
        bool use_gpu;
        bool enable_vad;             // Enable Whisper's built-in VAD
        int min_silence_duration_ms; // Minimum silence duration for speech boundaries (ms)
        int speech_pad_ms;           // Padding around speech segments (ms)
        const char *vad_model_path;  // NEW: Path to VAD model
    } whisper_bridge_params;

    // Result structure (plain C types only)
    typedef struct
    {
        char *text; // Allocated string - caller must free
        float confidence;
        int64_t start_time_ms;
        int64_t end_time_ms;
        bool success;
        char *error_msg; // Allocated string - caller must free on error
    } whisper_bridge_result;

    // API Functions
    whisper_bridge_context *whisper_bridge_init(whisper_bridge_params params);
    void whisper_bridge_free(whisper_bridge_context *ctx);

    whisper_bridge_result whisper_bridge_transcribe_audio(
        whisper_bridge_context *ctx,
        const float *audio_data,
        int audio_len,
        int sample_rate);

    void whisper_bridge_free_result(whisper_bridge_result *result);

    // Real-time processing
    typedef void (*whisper_bridge_callback)(const whisper_bridge_result *result, void *user_data);

    bool whisper_bridge_start_stream(
        whisper_bridge_context *ctx,
        whisper_bridge_callback callback,
        void *user_data);

    void whisper_bridge_add_audio(
        whisper_bridge_context *ctx,
        const float *audio_data,
        int audio_len,
        double timestamp);

    void whisper_bridge_stop_stream(whisper_bridge_context *ctx);

#ifdef __cplusplus
}
#endif