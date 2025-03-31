#include "audio_processor.h"
#include <iostream>
#include <cmath>

AudioProcessor::AudioProcessor()
    : m_stream(nullptr), m_sndFile(nullptr), m_isPlayingFile(false),
      m_sampleRate(44100), m_framesPerBuffer(1024), m_numChannels(2), m_numBands(64),
      m_currentDeviceIndex(-1), m_smoothingFactor(0.3f), m_normalizationFactor(5.0f),
      m_fftPlan(nullptr), m_fftIn(nullptr), m_fftOut(nullptr)
{
    // Initialize vectors with default sizes to prevent issues before first callback/processing
    m_audioData.resize(m_framesPerBuffer * m_numChannels, 0.0f);
    size_t fftSize = m_framesPerBuffer / 2 + 1;
    m_fftData.resize(fftSize, 0.0f);
    m_bandEnergies.resize(m_numBands, 0.0f);
    m_previousBandEnergies.resize(m_numBands, 0.0f);

    // Allocate FFT buffers - they will be resized in setInputDevice if needed
    m_fftIn = (float*) fftwf_malloc(sizeof(float) * m_framesPerBuffer);
    m_fftOut = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);

    // Create the initial plan (can be updated in setInputDevice)
    if (m_fftIn && m_fftOut) {
        m_fftPlan = fftwf_plan_dft_r2c_1d(m_framesPerBuffer, m_fftIn, m_fftOut, FFTW_ESTIMATE);
    } else {
        std::cerr << "Error allocating initial FFT buffers!" << std::endl;
        // Handle allocation failure appropriately (e.g., set an error state)
    }
}

AudioProcessor::~AudioProcessor() {
    if (m_stream) {
        Pa_CloseStream(m_stream);
    }
    if (m_sndFile) {
        sf_close(m_sndFile);
    }
    Pa_Terminate();
    fftwf_destroy_plan(m_fftPlan);
    fftwf_free(m_fftIn);
    fftwf_free(m_fftOut);
}

void AudioProcessor::refreshDeviceList() {
    m_availableDevices.clear();
    
    int numDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo && deviceInfo->maxInputChannels > 0) {
            AudioDevice device;
            device.index = i;
            device.name = deviceInfo->name;
            device.maxInputChannels = deviceInfo->maxInputChannels;
            device.maxOutputChannels = deviceInfo->maxOutputChannels;
            device.defaultSampleRate = deviceInfo->defaultSampleRate;
            m_availableDevices.push_back(device);
        }
    }
}

void AudioProcessor::closeStream() {
    if (m_stream) {
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }
}

bool AudioProcessor::openStream(PaDeviceIndex deviceIndex) {
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if (!deviceInfo) {
        std::cerr << "Invalid device index" << std::endl;
        return false;
    }

    // Adjust channels if needed
    if (deviceInfo->maxInputChannels < m_numChannels) {
        std::cout << "Warning: Device does not support " << m_numChannels << " channels. "
                  << "Using " << deviceInfo->maxInputChannels << " channels instead." << std::endl;
        m_numChannels = deviceInfo->maxInputChannels;
    }

    PaStreamParameters inputParameters;
    inputParameters.device = deviceIndex;
    inputParameters.channelCount = m_numChannels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(&m_stream,
                               &inputParameters,
                               NULL,
                               m_sampleRate,
                               m_framesPerBuffer,
                               paClipOff,
                               paCallback,
                               this);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Resize audio data buffer based on actual channel count and buffer size
    m_numChannels = inputParameters.channelCount; // Update channel count based on device capability
    m_audioData.resize(m_framesPerBuffer * m_numChannels, 0.0f);

    return true;
}

bool AudioProcessor::switchToDevice(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_availableDevices.size())) {
        return false;
    }

    closeStream();
    
    if (!openStream(m_availableDevices[deviceIndex].index)) {
        return false;
    }

    m_currentDeviceIndex = deviceIndex;
    std::cout << "Switched to audio device: " << m_availableDevices[deviceIndex].name << std::endl;
    return true;
}

bool AudioProcessor::initialize(int sampleRate, int framesPerBuffer, int numChannels) {
    // Store parameters
    m_sampleRate = sampleRate;
    m_framesPerBuffer = framesPerBuffer;
    m_numChannels = numChannels;

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Error initializing PortAudio: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Get available input devices
    m_inputDevices.clear();
    int numDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels > 0) {
            m_inputDevices.push_back(deviceInfo->name);
        }
    }

    // Use default input device initially
    return setInputDevice(Pa_GetDefaultInputDevice());
}

bool AudioProcessor::setInputDevice(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= Pa_GetDeviceCount()) {
        return false;
    }

    // Close existing stream if any
    if (m_stream) {
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if (!deviceInfo) return false;

    // Configure stream parameters
    PaStreamParameters inputParams;
    inputParams.device = deviceIndex;
    inputParams.channelCount = std::min(2, deviceInfo->maxInputChannels);
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    // Open new stream
    PaError err = Pa_OpenStream(
        &m_stream,
        &inputParams,
        nullptr,  // No output
        m_sampleRate,
        m_framesPerBuffer,
        paClipOff,
        paCallback,
        this
    );

    if (err != paNoError) {
        std::cerr << "Error opening stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Start the stream
    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Error starting stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Resize audio data buffer based on actual channel count and buffer size
    m_numChannels = inputParams.channelCount; // Update channel count based on device capability
    m_audioData.resize(m_framesPerBuffer * m_numChannels, 0.0f);

    // Re-allocate FFT buffers and recreate plan if parameters changed
    size_t fftSize = m_framesPerBuffer / 2 + 1;
    if (m_fftIn) fftwf_free(m_fftIn);
    if (m_fftOut) fftwf_free(m_fftOut);
    m_fftIn = (float*) fftwf_malloc(sizeof(float) * m_framesPerBuffer);
    m_fftOut = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);

    if (m_fftPlan) fftwf_destroy_plan(m_fftPlan);
    if (m_fftIn && m_fftOut) {
        m_fftPlan = fftwf_plan_dft_r2c_1d(m_framesPerBuffer, m_fftIn, m_fftOut, FFTW_ESTIMATE);
    } else {
        std::cerr << "Error allocating FFT buffers in setInputDevice!" << std::endl;
        // Handle error
        return false;
    }

    std::cout << "Audio processor initialized successfully:\n"
              << "- Using device: " << deviceInfo->name << "\n"
              << "- Channels: " << inputParams.channelCount << "\n"
              << "- Sample rate: " << m_sampleRate << "\n"
              << "- Buffer size: " << m_framesPerBuffer << std::endl;

    return true;
}

void AudioProcessor::processAudio() {
    // Check if audio data is available
    if (m_audioData.empty()) {
        return; // Nothing to process
    }

    // Resize FFT buffers if needed
    size_t fftSize = m_framesPerBuffer / 2 + 1;
    if (m_fftData.size() != fftSize) {
        m_fftData.resize(fftSize);
    }
    if (m_bandEnergies.size() != static_cast<size_t>(m_numBands)) {
        m_bandEnergies.resize(m_numBands, 0.0f);
        m_previousBandEnergies.resize(m_numBands, 0.0f);
    }

    // Check if FFT buffers and plan are ready
    if (!m_fftIn || !m_fftOut || !m_fftPlan) {
        std::cerr << "FFT resources not initialized!" << std::endl;
        return;
    }

    // Prepare FFT input (using m_audioData and member buffer m_fftIn)
    for (int i = 0; i < m_framesPerBuffer; ++i) {
        // Use only the first channel for simplicity, or average channels if m_numChannels > 1
        size_t data_index = static_cast<size_t>(i) * m_numChannels;
        m_fftIn[i] = (m_numChannels > 0 && data_index < m_audioData.size()) ? m_audioData[data_index] : 0.0f;
    }

    // Execute FFT using member plan and buffers
    fftwf_execute(m_fftPlan);

    // Process FFT output (m_fftOut) to get band energies
    for (int i = 0; i < m_numBands; i++) {
        float sum = 0;
        // Logarithmic frequency bands calculation (adjust as needed)
        int start_k = static_cast<int>(pow(2, static_cast<float>(i) / m_numBands * log2(fftSize)));
        int end_k = static_cast<int>(pow(2, static_cast<float>(i + 1) / m_numBands * log2(fftSize)));
        start_k = std::max(1, std::min(start_k, (int)fftSize - 1)); // Clamp indices
        end_k = std::max(1, std::min(end_k, (int)fftSize));
        if (start_k >= end_k) continue;

        for (int k = start_k; k < end_k; k++) {
            float real = m_fftOut[k][0]; // Use m_fftOut
            float imag = m_fftOut[k][1]; // Use m_fftOut
            float magnitude = sqrt(real * real + imag * imag);
            sum += magnitude;
        }

        float currentEnergy = (end_k > start_k) ? (sum / (end_k - start_k)) : 0.0f;
        m_bandEnergies[i] = m_smoothingFactor * currentEnergy + 
                            (1.0f - m_smoothingFactor) * m_previousBandEnergies[i];
        m_previousBandEnergies[i] = m_bandEnergies[i];
        m_bandEnergies[i] *= m_normalizationFactor;
    }

    // No need to free local buffers anymore
    // fftwf_free(fftIn);
    // fftwf_free(fftOut);
}

void AudioProcessor::toggleAudioSource() {
    m_isPlayingFile = !m_isPlayingFile;
    if (!m_isPlayingFile && m_sndFile) {
        sf_close(m_sndFile);
        m_sndFile = nullptr;
    }
}

bool AudioProcessor::loadAudioFile(const std::string& filePath) {
    if (m_sndFile) {
        sf_close(m_sndFile);
    }

    m_sndFile = sf_open(filePath.c_str(), SFM_READ, &m_sfInfo);
    if (!m_sndFile) {
        std::cerr << "Error opening audio file: " << filePath << std::endl;
        return false;
    }

    m_isPlayingFile = true;
    return true;
}

int AudioProcessor::paCallback(const void* inputBuffer, [[maybe_unused]] void* outputBuffer,
                               unsigned long framesPerBuffer,
                               [[maybe_unused]] const PaStreamCallbackTimeInfo* timeInfo,
                               [[maybe_unused]] PaStreamCallbackFlags statusFlags,
                               void* userData) {
    AudioProcessor* processor = static_cast<AudioProcessor*>(userData);
    float* in = (float*)inputBuffer;
    
    if (processor->m_isPlayingFile && processor->m_sndFile) {
        sf_count_t count = sf_read_float(processor->m_sndFile, processor->m_audioData.data(), framesPerBuffer * processor->m_numChannels);
        if (static_cast<unsigned long>(count) < framesPerBuffer * processor->m_numChannels) {
            sf_seek(processor->m_sndFile, 0, SEEK_SET);  // Loop back to the beginning of the file
        }
    } else {
        for (unsigned long i = 0; i < framesPerBuffer * processor->m_numChannels; i++) {
            processor->m_audioData[i] = in[i];
        }
    }

    return paContinue;
}

void AudioProcessor::setFilePlayback(bool play) {
    if (!m_audioFile) return;
    m_isPlayingFile = play;
    if (!play) {
        // Reset file position when paused
        sf_seek(m_audioFile, 0, SEEK_SET);
    }
}