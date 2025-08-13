#include "WhisperTranscriber.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

WhisperTranscriber::WhisperTranscriber(const Config &config)
    : config_(config), whisperContext_(nullptr), initialized_(false), shouldStop_(false), bufferStartTime_(0.0)
{
    // Initialize audio buffer
    const size_t bufferSamples = BUFFER_SIZE_SECONDS * 16000; // 16kHz * seconds
    audioBuffer_.reserve(bufferSamples);
}

WhisperTranscriber::~WhisperTranscriber()
{
    stopRealTimeProcessing();

    if (whisperContext_)
    {
        whisper_bridge_free(whisperContext_);
        whisperContext_ = nullptr;
    }
}

bool WhisperTranscriber::initialize()
{
    if (initialized_)
    {
        return true;
    }

    std::cout << "Loading Whisper model: " << config_.modelPath << std::endl;

    // Initialize whisper bridge parameters
    whisper_bridge_params params = {};
    params.model_path = config_.modelPath.c_str();
    params.language = config_.language.c_str();
    params.threads = config_.threads;
    params.max_len_ms = config_.maxSegmentLength * 1000;
    params.vad_threshold = config_.silenceThreshold;
    params.use_gpu = false; // Use CPU for compatibility

    // Initialize the bridge
    whisperContext_ = whisper_bridge_init(params);

    if (!whisperContext_)
    {
        std::cerr << "Failed to initialize Whisper context from: " << config_.modelPath << std::endl;
        return false;
    }

    initialized_ = true;

    // Print system info if in debug mode
    printSystemInfo();

    std::cout << "Whisper model loaded successfully!" << std::endl;

    return true;
}

std::vector<WhisperTranscriber::Result> WhisperTranscriber::transcribe(const std::vector<float> &audioData)
{
    if (!initialized_ || audioData.empty())
    {
        return {};
    }

    // Use the bridge API for transcription
    whisper_bridge_result result = whisper_bridge_transcribe_audio(
        whisperContext_, 
        audioData.data(), 
        audioData.size(), 
        16000  // sample rate
    );

    if (!result.success)
    {
        std::cerr << "Failed to process audio with Whisper: " 
                  << (result.error_msg ? result.error_msg : "Unknown error") << std::endl;
        whisper_bridge_free_result(&result);
        return {};
    }

    // Extract results
    auto results = extractResults(result);
    whisper_bridge_free_result(&result);
    return results;
}

void WhisperTranscriber::addAudioData(const std::vector<float> &audioData, double timestamp)
{
    if (!initialized_ || audioData.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(queueMutex_);
    audioQueue_.push(std::make_pair(audioData, timestamp));
    queueCondition_.notify_one();
}

void WhisperTranscriber::startRealTimeProcessing(std::function<void(const Result &)> callback)
{
    if (processingThread_.joinable())
    {
        return; // Already running
    }

    resultCallback_ = callback;
    shouldStop_.store(false);

    processingThread_ = std::thread(&WhisperTranscriber::processingThreadFunction, this);

    std::cout << "Real-time processing started" << std::endl;
}

void WhisperTranscriber::stopRealTimeProcessing()
{
    if (!processingThread_.joinable())
    {
        return;
    }

    shouldStop_.store(true);
    queueCondition_.notify_all();

    if (processingThread_.joinable())
    {
        processingThread_.join();
    }

    // Clear remaining data
    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!audioQueue_.empty())
    {
        audioQueue_.pop();
    }
    audioBuffer_.clear();

    std::cout << "Real-time processing stopped" << std::endl;
}

bool WhisperTranscriber::isInitialized() const
{
    return initialized_;
}

std::vector<std::string> WhisperTranscriber::getSupportedLanguages()
{
    return {
        "auto", "en", "zh", "de", "es", "ru", "ko", "fr", "ja", "pt", "tr", "pl", "ca", "nl",
        "ar", "sv", "it", "id", "hi", "fi", "vi", "he", "uk", "el", "ms", "cs", "ro", "da",
        "hu", "ta", "no", "th", "ur", "hr", "bg", "lt", "la", "mi", "ml", "cy", "sk", "te",
        "fa", "lv", "bn", "sr", "az", "sl", "kn", "et", "mk", "br", "eu", "is", "hy", "ne",
        "mn", "bs", "kk", "sq", "sw", "gl", "mr", "pa", "si", "km", "sn", "yo", "so", "af",
        "oc", "ka", "be", "tg", "sd", "gu", "am", "yi", "lo", "uz", "fo", "ht", "ps", "tk",
        "nn", "mt", "sa", "lb", "my", "bo", "tl", "mg", "as", "tt", "haw", "ln", "ha", "ba",
        "jw", "su"};
}

void WhisperTranscriber::setLanguage(const std::string &language)
{
    config_.language = language;
}

void WhisperTranscriber::processingThreadFunction()
{
    std::cout << "Processing thread started" << std::endl;

    while (!shouldStop_.load())
    {
        std::unique_lock<std::mutex> lock(queueMutex_);

        // Wait for audio data or stop signal
        queueCondition_.wait_for(lock, std::chrono::milliseconds(100), [this]()
                                 { return !audioQueue_.empty() || shouldStop_.load(); });

        if (shouldStop_.load())
        {
            break;
        }

        // Process available audio data
        while (!audioQueue_.empty() && !shouldStop_.load())
        {
            auto audioData = audioQueue_.front();
            audioQueue_.pop();
            lock.unlock();

            // Add to buffer
            audioBuffer_.insert(audioBuffer_.end(),
                                audioData.first.begin(),
                                audioData.first.end());

            // Set buffer start time if this is the first chunk
            if (bufferStartTime_ == 0.0)
            {
                bufferStartTime_ = audioData.second;
            }

            // Check if we should process the buffer
            const size_t minSamples = MIN_PROCESS_SIZE_SECONDS * 16000;
            const size_t maxSamples = BUFFER_SIZE_SECONDS * 16000;

            bool shouldProcess = false;

            // Process if buffer is getting full
            if (audioBuffer_.size() >= maxSamples)
            {
                shouldProcess = true;
            }
            // Or if we have enough audio and detect silence/end of speech
            else if (audioBuffer_.size() >= minSamples)
            {
                // Simple speech detection - check if recent audio is quiet
                if (detectSpeech(audioData.first))
                {
                    // Continue accumulating if speech is ongoing
                }
                else
                {
                    shouldProcess = true;
                }
            }

            if (shouldProcess)
            {
                processBuffer();
            }

            lock.lock();
        }
    }

    // Process any remaining buffer
    if (!audioBuffer_.empty())
    {
        processBuffer();
    }

    std::cout << "Processing thread ended" << std::endl;
}

bool WhisperTranscriber::processBuffer()
{
    if (audioBuffer_.empty() || !resultCallback_)
    {
        return false;
    }

    // Create a copy of the buffer for processing
    std::vector<float> audioToProcess = audioBuffer_;
    double startTime = bufferStartTime_;

    // Clear the buffer for new audio
    audioBuffer_.clear();
    bufferStartTime_ = 0.0;

    // Transcribe the audio
    auto results = transcribe(audioToProcess);

    // Send results to callback
    for (const auto &result : results)
    {
        if (!result.text.empty() && resultCallback_)
        {
            // Adjust timestamps relative to the buffer start
            Result adjustedResult = result;
            adjustedResult.startTime += startTime;
            adjustedResult.endTime += startTime;

            resultCallback_(adjustedResult);
        }
    }

    return true;
}

bool WhisperTranscriber::detectSpeech(const std::vector<float> &audioData) const
{
    if (audioData.empty())
    {
        return false;
    }

    // Simple energy-based speech detection
    float energy = 0.0f;
    for (float sample : audioData)
    {
        energy += sample * sample;
    }
    energy /= audioData.size();

    // Return true if energy is above threshold (indicates speech)
    return energy > (config_.silenceThreshold * config_.silenceThreshold);
}

std::vector<WhisperTranscriber::Result> WhisperTranscriber::extractResults(const whisper_bridge_result &bridge_result) const
{
    std::vector<Result> results;

    if (bridge_result.text && strlen(bridge_result.text) > 0)
    {
        Result result;
        result.text = bridge_result.text;
        result.startTime = bridge_result.start_time_ms / 1000.0; // Convert from milliseconds to seconds
        result.endTime = bridge_result.end_time_ms / 1000.0;
        result.confidence = bridge_result.confidence;
        result.language = config_.language;

        // Trim whitespace
        result.text.erase(result.text.begin(),
                          std::find_if(result.text.begin(), result.text.end(),
                                       [](unsigned char ch)
                                       { return !std::isspace(ch); }));
        result.text.erase(std::find_if(result.text.rbegin(), result.text.rend(),
                                       [](unsigned char ch)
                                       { return !std::isspace(ch); })
                              .base(),
                          result.text.end());

        if (!result.text.empty())
        {
            results.push_back(result);
        }
    }

    return results;
}

void WhisperTranscriber::printSystemInfo() const
{
    if (!whisperContext_)
    {
        return;
    }

    // Print whisper system information
    std::cout << "Whisper system info:" << std::endl;
    std::cout << "  Sample rate: 16000 Hz" << std::endl;
    std::cout << "  Channels: 1 (mono)" << std::endl;
    std::cout << "  Language: " << (config_.language == "auto" ? "auto-detect" : config_.language) << std::endl;
    std::cout << "  Threads: " << config_.threads << std::endl;
    std::cout << "  Model: " << config_.modelPath << std::endl;
}