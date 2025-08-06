#include "AudioBuffer.h"
#include <algorithm>

AudioBuffer::AudioBuffer(size_t sizeInSamples)
    : buffer_(sizeInSamples), size_(sizeInSamples), writeIndex_(0), readIndex_(0), availableSamples_(0)
{
}

size_t AudioBuffer::write(const float *data, size_t numSamples)
{
    if (!data || numSamples == 0)
    {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    size_t samplesWritten = 0;
    size_t freeSamples = size_ - availableSamples_;
    size_t samplesToWrite = std::min(numSamples, freeSamples);

    for (size_t i = 0; i < samplesToWrite; i++)
    {
        buffer_[writeIndex_] = data[i];
        writeIndex_ = nextIndex(writeIndex_);
        samplesWritten++;
    }

    availableSamples_ += samplesWritten;

    return samplesWritten;
}

size_t AudioBuffer::read(float *data, size_t numSamples)
{
    if (!data || numSamples == 0)
    {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    size_t samplesRead = 0;
    size_t samplesToRead = std::min(numSamples, availableSamples_);

    for (size_t i = 0; i < samplesToRead; i++)
    {
        data[i] = buffer_[readIndex_];
        readIndex_ = nextIndex(readIndex_);
        samplesRead++;
    }

    availableSamples_ -= samplesRead;

    return samplesRead;
}

size_t AudioBuffer::getAvailableSamples() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return availableSamples_;
}

size_t AudioBuffer::getFreeSamples() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return size_ - availableSamples_;
}

bool AudioBuffer::isEmpty() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return availableSamples_ == 0;
}

bool AudioBuffer::isFull() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return availableSamples_ == size_;
}

void AudioBuffer::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    writeIndex_ = 0;
    readIndex_ = 0;
    availableSamples_ = 0;
}

size_t AudioBuffer::getSize() const
{
    return size_;
}

size_t AudioBuffer::nextIndex(size_t index) const
{
    return (index + 1) % size_;
}