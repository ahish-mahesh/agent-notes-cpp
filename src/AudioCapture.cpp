#include "AudioCapture.h"
#include <iostream>
#include <cstring>
#include <algorithm>

AudioCapture::AudioCapture()
    : AudioCapture(Config{}) // Delegate to the parametrized constructor
{
}

AudioCapture::AudioCapture(const Config &config)
    : config_(config), isCapturing_(false), audioBuffer_(nullptr)
{
#ifdef USE_RTAUDIO
    // Initialize RtAudio here
    try
    {
        rtAudio_ = std::make_unique<RtAudio>();
        std::cout << "RtAudio initialized with " << rtAudio_->getDeviceCount() << " devices" << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "Failed to initialize RtAudio: " << e.what() << std::endl;
        rtAudio_ = nullptr;
    }
#endif

#ifdef USE_PORTAUDIO
    paStream_ = nullptr;
#endif
}

AudioCapture::~AudioCapture()
{
    stop();

#ifdef USE_PORTAUDIO
    if (paStream_)
    {
        Pa_CloseStream(paStream_);
    }
    Pa_Terminate();
#endif
}

bool AudioCapture::initialize()
{
#ifdef USE_RTAUDIO
    if (!rtAudio_)
    {
        std::cerr << "RtAudio not initialized!" << std::endl;
        return false;
    }
    return initializeRtAudio();
#elif defined(USE_PORTAUDIO)
    return initializePortAudio();
#else
    std::cerr << "No audio library configured!" << std::endl;
    return false;
#endif
}

#ifdef USE_RTAUDIO
bool AudioCapture::initializeRtAudio()
{
    if (rtAudio_->getDeviceCount() < 1)
    {
        std::cerr << "No audio devices found!" << std::endl;
        return false;
    }

    // If no specific device ID is set, find the default input device
    if (config_.deviceId == 0 || config_.deviceId >= rtAudio_->getDeviceCount())
    {
        // Find the first available input device
        bool foundInputDevice = false;
        unsigned int deviceCount = rtAudio_->getDeviceCount();

        for (unsigned int i = 0; i < deviceCount; i++)
        {
            RtAudio::DeviceInfo info = rtAudio_->getDeviceInfo(i);
            if (info.inputChannels > 0)
            {
                config_.deviceId = i;
                foundInputDevice = true;
                std::cout << "Using audio input device: " << info.name << " (ID: " << i << ")" << std::endl;
                break;
            }
        }

        if (!foundInputDevice)
        {
            std::cerr << "No input devices found!" << std::endl;
            return false;
        }
    }
    else
    {
        // Validate the specified device ID
        RtAudio::DeviceInfo info = rtAudio_->getDeviceInfo(config_.deviceId);
        if (info.inputChannels == 0)
        {
            std::cerr << "Device ID " << config_.deviceId << " has no input channels!" << std::endl;
            return false;
        }
        std::cout << "Using specified audio input device: " << info.name << " (ID: " << config_.deviceId << ")" << std::endl;
    }

    // Create audio buffer
    audioBuffer_ = std::make_unique<AudioBuffer>(config_.sampleRate * 2); // 2 seconds buffer

    return true;
}

int AudioCapture::rtAudioCallback(void *outputBuffer, void *inputBuffer,
                                  unsigned int nFrames, double streamTime,
                                  RtAudioStreamStatus status, void *userData)
{

    // Suppress unused parameter warnings
    (void)outputBuffer; // suppress unused parameter warning

    auto *capture = static_cast<AudioCapture *>(userData);

    if (status)
    {
        std::cerr << "Stream underflow detected!" << std::endl;
    }

    if (capture && inputBuffer && capture->isCapturing_.load())
    {
        capture->processAudioData(inputBuffer, nFrames, streamTime);
    }

    return 0;
}
#endif

#ifdef USE_PORTAUDIO
bool AudioCapture::initializePortAudio()
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Create audio buffer
    audioBuffer_ = std::make_unique<AudioBuffer>(config_.sampleRate * 2); // 2 seconds buffer

    return true;
}

int AudioCapture::portAudioCallback(const void *inputBuffer, void *outputBuffer,
                                    unsigned long framesPerBuffer,
                                    const PaStreamCallbackTimeInfo *timeInfo,
                                    PaStreamCallbackFlags statusFlags,
                                    void *userData)
{

    auto *capture = static_cast<AudioCapture *>(userData);

    if (statusFlags)
    {
        std::cerr << "PortAudio status flags: " << statusFlags << std::endl;
    }

    if (capture && inputBuffer && capture->isCapturing_.load())
    {
        capture->processAudioData(inputBuffer, framesPerBuffer, timeInfo->inputBufferAdcTime);
    }

    return paContinue;
}
#endif

bool AudioCapture::start(AudioCallback callback)
{
    if (isCapturing_.load())
    {
        return true; // Already capturing
    }

    callback_ = callback;

#ifdef USE_RTAUDIO
    return startRtAudio();
#elif defined(USE_PORTAUDIO)
    return startPortAudio();
#else
    return false;
#endif
}

#ifdef USE_RTAUDIO
bool AudioCapture::startRtAudio()
{
    // Validate device before opening stream
    if (config_.deviceId >= rtAudio_->getDeviceCount())
    {
        std::cerr << "Invalid device ID: " << config_.deviceId << std::endl;
        return false;
    }

    RtAudio::DeviceInfo deviceInfo = rtAudio_->getDeviceInfo(config_.deviceId);
    if (deviceInfo.inputChannels == 0)
    {
        std::cerr << "Selected device has no input channels!" << std::endl;
        return false;
    }

    // Ensure we don't request more channels than the device supports
    if (config_.channels > deviceInfo.inputChannels)
    {
        std::cout << "Reducing channels from " << config_.channels << " to " << deviceInfo.inputChannels << std::endl;
        config_.channels = deviceInfo.inputChannels;
    }

    RtAudio::StreamParameters inputParams;
    inputParams.deviceId = config_.deviceId;
    inputParams.nChannels = config_.channels;
    inputParams.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_SCHEDULE_REALTIME;
    options.priority = 1;

    try
    {
        rtAudio_->openStream(
            nullptr,             // No output
            &inputParams,        // Input parameters
            RTAUDIO_FLOAT32,     // Sample format
            config_.sampleRate,  // Sample rate
            &config_.bufferSize, // Buffer size
            &rtAudioCallback,    // Callback
            this,                // User data
            &options             // Stream options
        );

        rtAudio_->startStream();
        isCapturing_.store(true);

        std::cout << "RtAudio stream started successfully" << std::endl;
        return true;
    }
    catch (std::exception &e)
    {
        std::cerr << "RtAudio error: " << e.what() << std::endl;
        return false;
    }
}
#endif

#ifdef USE_PORTAUDIO
bool AudioCapture::startPortAudio()
{
    PaStreamParameters inputParameters;
    inputParameters.device = config_.deviceId;
    if (inputParameters.device == paNoDevice)
    {
        inputParameters.device = Pa_GetDefaultInputDevice();
    }

    inputParameters.channelCount = config_.channels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(
        &paStream_,
        &inputParameters,
        nullptr, // No output
        config_.sampleRate,
        config_.bufferSize,
        paClipOff, // No clipping
        portAudioCallback,
        this);

    if (err != paNoError)
    {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    err = Pa_StartStream(paStream_);
    if (err != paNoError)
    {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    isCapturing_.store(true);
    std::cout << "PortAudio stream started successfully" << std::endl;
    return true;
}
#endif

void AudioCapture::stop()
{
    if (!isCapturing_.load())
    {
        return;
    }

    isCapturing_.store(false);

#ifdef USE_RTAUDIO
    if (rtAudio_ && rtAudio_->isStreamOpen())
    {
        rtAudio_->stopStream();
        rtAudio_->closeStream();
    }
#endif

#ifdef USE_PORTAUDIO
    if (paStream_)
    {
        Pa_StopStream(paStream_);
        Pa_CloseStream(paStream_);
        paStream_ = nullptr;
    }
#endif
}

bool AudioCapture::isCapturing() const
{
    return isCapturing_.load();
}

std::vector<std::string> AudioCapture::getAvailableDevices() const
{
    std::vector<std::string> devices;

#ifdef USE_RTAUDIO
    if (rtAudio_)
    {
        unsigned int deviceCount = rtAudio_->getDeviceCount();
        for (unsigned int i = 0; i < deviceCount; i++)
        {
            RtAudio::DeviceInfo info = rtAudio_->getDeviceInfo(i);
            if (info.inputChannels > 0)
            {
                devices.push_back(info.name);
            }
        }
    }
#endif

#ifdef USE_PORTAUDIO
    int deviceCount = Pa_GetDeviceCount();
    for (int i = 0; i < deviceCount; i++)
    {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0)
        {
            devices.push_back(info->name);
        }
    }
#endif

    return devices;
}

void AudioCapture::printAvailableDevices() const
{
#ifdef USE_RTAUDIO
    if (rtAudio_)
    {
        unsigned int deviceCount = rtAudio_->getDeviceCount();
        std::cout << "Available audio devices:" << std::endl;

        for (unsigned int i = 0; i < deviceCount; i++)
        {
            RtAudio::DeviceInfo info = rtAudio_->getDeviceInfo(i);
            std::cout << "  Device " << i << ": " << info.name
                      << " (Input channels: " << info.inputChannels
                      << ", Output channels: " << info.outputChannels << ")" << std::endl;
        }
    }
#endif

#ifdef USE_PORTAUDIO
    int deviceCount = Pa_GetDeviceCount();
    std::cout << "Available audio devices:" << deviceCount << std::endl;

    for (int i = 0; i < deviceCount; i++)
    {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0)
        {
            std::cout << "  Device " << i << ": " << info->name
                      << " (Input channels: " << info->maxInputChannels
                      << ", Output channels: " << info->maxOutputChannels << ")" << std::endl;
        }
    }
#endif
}

bool AudioCapture::setDevice(unsigned int deviceId)
{
    if (isCapturing_.load())
    {
        return false; // Can't change device while capturing
    }

    config_.deviceId = deviceId;
    return true;
}

void AudioCapture::processAudioData(const void *inputBuffer, unsigned int frames, double timestamp)
{
    if (!inputBuffer || !callback_)
    {
        return;
    }

    // Convert input to float if necessary
    std::vector<float> floatData = convertToFloat(inputBuffer, frames, RTAUDIO_FLOAT32);

    // Call the user callback
    if (!floatData.empty())
    {
        callback_(floatData, timestamp);
    }
}

std::vector<float> AudioCapture::convertToFloat(const void *input, unsigned int frames, int format) const
{
    std::vector<float> output;
    output.reserve(frames * config_.channels);

    if (format == RTAUDIO_FLOAT32)
    {
        const float *floatInput = static_cast<const float *>(input);
        output.assign(floatInput, floatInput + frames * config_.channels);
    }
    else if (format == RTAUDIO_SINT16)
    {
        const int16_t *shortInput = static_cast<const int16_t *>(input);
        for (unsigned int i = 0; i < frames * config_.channels; i++)
        {
            output.push_back(static_cast<float>(shortInput[i]) / 32768.0f);
        }
    }
    else if (format == RTAUDIO_SINT32)
    {
        const int32_t *intInput = static_cast<const int32_t *>(input);
        for (unsigned int i = 0; i < frames * config_.channels; i++)
        {
            output.push_back(static_cast<float>(intInput[i]) / 2147483648.0f);
        }
    }

    // Convert to mono if needed
    if (config_.channels > 1)
    {
        std::vector<float> mono;
        mono.reserve(frames);

        for (unsigned int i = 0; i < frames; i++)
        {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < config_.channels; ch++)
            {
                sum += output[i * config_.channels + ch];
            }
            mono.push_back(sum / config_.channels);
        }

        return mono;
    }

    return output;
}