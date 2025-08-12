#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>

#include "RtAudio.h"

#ifdef USE_PORTAUDIO
#include "portaudio.h"
#endif

#include "AudioBuffer.h"

/**
 * @brief Cross-platform audio capture class using RtAudio or PortAudio
 *
 * Handles real-time audio capture from the system's default input device.
 * Automatically converts audio to the format required by Whisper (16kHz, mono, float32).
 */
class AudioCapture
{
public:
    /**
     * @brief Configuration for audio capture
     */
    struct Config
    {
        unsigned int sampleRate = 16000; ///< Target sample rate for Whisper
        unsigned int channels = 1;       ///< Mono audio
        unsigned int bufferSize = 128;   ///< Audio buffer size in frames
        unsigned int deviceId = 0;       ///< Audio device ID (0 = default)

        /**
         * @brief Default constructor
         */
        Config() = default;
    };

    /**
     * @brief Callback function type for processed audio data
     * @param audioData Vector of float samples (mono, 16kHz)
     * @param timestamp Timestamp when audio was captured
     */
    using AudioCallback = std::function<void(const std::vector<float> &, double)>;

    /**
     * @brief Default constructor with default configuration
     */
    AudioCapture();

    /**
     * @brief Constructor
     * @param config Audio capture configuration
     */
    explicit AudioCapture(const Config &config);

    /**
     * @brief Destructor
     */
    ~AudioCapture();

    /**
     * @brief Initialize the audio capture system
     * @return true on success, false on failure
     */
    bool initialize();

    /**
     * @brief Start audio capture
     * @param callback Function to call with captured audio data
     * @return true on success, false on failure
     */
    bool start(AudioCallback callback);

    /**
     * @brief Stop audio capture
     */
    void stop();

    /**
     * @brief Check if audio capture is currently active
     * @return true if capturing, false otherwise
     */
    bool isCapturing() const;

    /**
     * @brief Get list of available audio input devices
     * @return Vector of device names
     */
    std::vector<std::string> getAvailableDevices() const;

    /**
     * @brief Set the input device to use
     * @param deviceId Device ID to use (from getAvailableDevices)
     * @return true on success, false if device not available
     */
    bool setDevice(unsigned int deviceId);

    /**
     * @brief Print available audio devices
     */
    void printAvailableDevices() const;

private:
    Config config_;
    AudioCallback callback_;
    std::atomic<bool> isCapturing_;
    std::unique_ptr<AudioBuffer> audioBuffer_;

#ifdef USE_RTAUDIO
    std::unique_ptr<RtAudio> rtAudio_;

    /**
     * @brief Initialize RtAudio
     */
    bool initializeRtAudio();

    /**
     * @brief Start RtAudio stream
     */
    bool startRtAudio();

    /**
     * @brief RtAudio callback function
     */
    static int rtAudioCallback(void *outputBuffer, void *inputBuffer,
                               unsigned int nFrames, double streamTime,
                               RtAudioStreamStatus status, void *userData);
#endif

#ifdef USE_PORTAUDIO
    PaStream *paStream_;

    /**
     * @brief Initialize PortAudio
     */
    bool initializePortAudio();

    /**
     * @brief Start PortAudio stream
     */
    bool startPortAudio();

    /**
     * @brief PortAudio callback function
     */
    static int portAudioCallback(const void *inputBuffer, void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData);
#endif

    /**
     * @brief Process raw audio data and convert to required format
     * @param inputBuffer Raw audio data
     * @param frames Number of audio frames
     * @param timestamp Audio timestamp
     */
    void processAudioData(const void *inputBuffer, unsigned int frames, double timestamp);

    /**
     * @brief Convert audio samples to float format
     * @param input Input samples (various formats)
     * @param frames Number of frames
     * @param format Input sample format
     * @return Vector of float samples
     */
    std::vector<float> convertToFloat(const void *input, unsigned int frames, int format) const;
};