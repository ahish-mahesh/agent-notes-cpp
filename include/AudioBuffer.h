#pragma once

#include <vector>
#include <mutex>
#include <memory>

/**
 * @brief Thread-safe circular buffer for audio data
 *
 * Provides a lock-free ring buffer optimized for real-time audio processing.
 */
class AudioBuffer
{
public:
    /**
     * @brief Constructor
     * @param sizeInSamples Buffer size in audio samples
     */
    explicit AudioBuffer(size_t sizeInSamples);

    /**
     * @brief Destructor
     */
    ~AudioBuffer() = default;

    /**
     * @brief Write audio data to the buffer
     * @param data Pointer to audio data
     * @param numSamples Number of samples to write
     * @return Number of samples actually written
     */
    size_t write(const float *data, size_t numSamples);

    /**
     * @brief Read audio data from the buffer
     * @param data Pointer to output buffer
     * @param numSamples Number of samples to read
     * @return Number of samples actually read
     */
    size_t read(float *data, size_t numSamples);

    /**
     * @brief Get the number of samples available for reading
     * @return Number of available samples
     */
    size_t getAvailableSamples() const;

    /**
     * @brief Get the number of samples available for writing
     * @return Number of free samples
     */
    size_t getFreeSamples() const;

    /**
     * @brief Check if the buffer is empty
     * @return true if empty, false otherwise
     */
    bool isEmpty() const;

    /**
     * @brief Check if the buffer is full
     * @return true if full, false otherwise
     */
    bool isFull() const;

    /**
     * @brief Clear all data from the buffer
     */
    void clear();

    /**
     * @brief Get total buffer size
     * @return Buffer size in samples
     */
    size_t getSize() const;

private:
    std::vector<float> buffer_;
    size_t size_;
    mutable std::mutex mutex_;
    size_t writeIndex_;
    size_t readIndex_;
    size_t availableSamples_;

    /**
     * @brief Get next index in circular buffer
     * @param index Current index
     * @return Next index (wrapped around if necessary)
     */
    size_t nextIndex(size_t index) const;
};