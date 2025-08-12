#pragma once

#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

#include "WhisperBridge.h"

/**
 * @brief Whisper-based speech transcription class
 *
 * Handles loading Whisper models and transcribing audio data.
 * Supports both real-time streaming and batch transcription.
 */
class WhisperTranscriber
{
public:
    /**
     * @brief Configuration for Whisper transcriber
     */
    struct Config
    {
        std::string modelPath;          ///< Path to Whisper model file
        int threads = 4;                ///< Number of threads for inference
        std::string language = "auto";  ///< Language code ("en", "auto", etc.)
        bool translate = false;         ///< Translate to English if source is not English
        float silenceThreshold = 0.01f; ///< Silence detection threshold
        int maxSegmentLength = 30;      ///< Maximum segment length in seconds
        bool enableVAD = true;          ///< Enable Voice Activity
        bool suppressNonSpeech = true;  ///< Suppress non-speech tokens
    };

    /**
     * @brief Transcription result structure
     */
    struct Result
    {
        std::string text;     ///< Transcribed text
        double startTime;     ///< Start time in seconds
        double endTime;       ///< End time in seconds
        float confidence;     ///< Confidence score (0.0 - 1.0)
        std::string language; ///< Detected language
    };

    /**
     * @brief Constructor
     * @param config Transcriber configuration
     */
    explicit WhisperTranscriber(const Config &config);

    /**
     * @brief Destructor
     */
    ~WhisperTranscriber();

    /**
     * @brief Initialize the transcriber (load model)
     * @return true on success, false on failure
     */
    bool initialize();

    /**
     * @brief Transcribe audio data
     * @param audioData Float audio samples (mono, 16kHz)
     * @return Transcription results
     */
    std::vector<Result> transcribe(const std::vector<float> &audioData);

    /**
     * @brief Add audio data to the transcription queue (for real-time processing)
     * @param audioData Float audio samples (mono, 16kHz)
     * @param timestamp Audio timestamp
     */
    void addAudioData(const std::vector<float> &audioData, double timestamp);

    /**
     * @brief Start real-time transcription processing
     * @param callback Function to call with transcription results
     */
    void startRealTimeProcessing(std::function<void(const Result &)> callback);

    /**
     * @brief Stop real-time transcription processing
     */
    void stopRealTimeProcessing();

    /**
     * @brief Check if the transcriber is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

    /**
     * @brief Get supported languages
     * @return Vector of language codes
     */
    static std::vector<std::string> getSupportedLanguages();

    /**
     * @brief Set transcription language
     * @param language Language code ("en", "es", "fr", etc.) or "auto"
     */
    void setLanguage(const std::string &language);

private:
    Config config_;
    whisper_bridge_context *whisperContext_;
    bool initialized_;

    // Real-time processing
    std::queue<std::pair<std::vector<float>, double>> audioQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::thread processingThread_;
    std::atomic<bool> shouldStop_;
    std::function<void(const Result &)> resultCallback_;

    // Audio buffering for real-time processing
    std::vector<float> audioBuffer_;
    double bufferStartTime_;
    static constexpr size_t BUFFER_SIZE_SECONDS = 10;     ///< Buffer size in seconds
    static constexpr size_t MIN_PROCESS_SIZE_SECONDS = 2; ///< Minimum audio length to process

    /**
     * @brief Real-time processing thread function
     */
    void processingThreadFunction();

    /**
     * @brief Process accumulated audio buffer
     * @return true if processing was successful
     */
    bool processBuffer();

    /**
     * @brief Detect if audio contains speech
     * @param audioData Audio samples to analyze
     * @return true if speech is detected
     */
    bool detectSpeech(const std::vector<float> &audioData) const;

    /**
     * @brief Convert whisper results to our Result structure
     * @param result Whisper bridge result
     * @return Vector of transcription results
     */
    std::vector<Result> extractResults(const whisper_bridge_result &result) const;

    /**
     * @brief Print system information and model details
     */
    void printSystemInfo() const;
};