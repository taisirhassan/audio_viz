#include "audio_processor.h"
#include <iostream>
#include <cmath>

AudioProcessor::AudioProcessor()
    : m_stream(nullptr), m_sndFile(nullptr), m_isPlayingFile(false),
      m_sampleRate(44100), m_framesPerBuffer(1024), m_numChannels(2), m_numBands(64),
      m_currentDeviceIndex(Pa_GetDefaultInputDevice()),
      m_activeSampleRate(44100),
      m_smoothingFactor(0.3f), m_normalizationFactor(5.0f),
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

bool AudioProcessor::openInputStream(PaDeviceIndex inputDeviceIndex, PaDeviceIndex outputDeviceIndex, int sampleRate) {
    closeStream(); // Ensure any existing stream is closed

    const PaDeviceInfo* inputDeviceInfo = Pa_GetDeviceInfo(inputDeviceIndex);
    const PaDeviceInfo* outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);

    if (!inputDeviceInfo || !outputDeviceInfo) {
        std::cerr << "Error getting device info in openInputStream." << std::endl;
        return false;
    }

    // Determine compatible channel count for IN->OUT
    int maxInputChannels = inputDeviceInfo->maxInputChannels;
    int maxOutputChannels = outputDeviceInfo->maxOutputChannels;
    int actualChannels = 0;
    if (maxInputChannels >= 2 && maxOutputChannels >= 2) actualChannels = 2;
    else if (maxInputChannels >= 1 && maxOutputChannels >= 1) actualChannels = 1;
    else {
        std::cerr << "Error: Could not find a compatible channel count for input/output devices." << std::endl;
        return false;
    }

    m_numChannels = actualChannels;
    m_activeSampleRate = sampleRate;

    PaStreamParameters inputParams;
    inputParams.device = inputDeviceIndex;
    inputParams.channelCount = m_numChannels;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = inputDeviceInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters outputParams;
    outputParams.device = outputDeviceIndex;
    outputParams.channelCount = m_numChannels;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(
        &m_stream,
        &inputParams,       // Input parameters
        &outputParams,      // Output parameters
        m_activeSampleRate, // Use provided sample rate
        m_framesPerBuffer,
        paClipOff,
        paCallback,
        this
    );

    if (err != paNoError) {
        std::cerr << "Error opening input stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Error starting input stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        return false;
    }

    m_audioData.resize(m_framesPerBuffer * m_numChannels, 0.0f);

    // Re-initialize FFT
    size_t fftSize = m_framesPerBuffer / 2 + 1;
    if (m_fftIn) fftwf_free(m_fftIn);
    if (m_fftOut) fftwf_free(m_fftOut);
    m_fftIn = (float*) fftwf_malloc(sizeof(float) * m_framesPerBuffer);
    m_fftOut = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    if (m_fftPlan) fftwf_destroy_plan(m_fftPlan);
    if (m_fftIn && m_fftOut) {
        m_fftPlan = fftwf_plan_dft_r2c_1d(m_framesPerBuffer, m_fftIn, m_fftOut, FFTW_ESTIMATE);
        if (!m_fftPlan) { /* error handling */ closeStream(); return false; }
    } else { /* error handling */ closeStream(); return false; }

    std::cout << "Input audio stream opened successfully:\n"
              << "- Input: " << inputDeviceInfo->name << "\n"
              << "- Output: " << outputDeviceInfo->name << "\n"
              << "- Channels: " << m_numChannels << "\n"
              << "- Sample Rate: " << m_activeSampleRate << "\n"
              << "- Buffer Size: " << m_framesPerBuffer << std::endl;

    return true;
}

bool AudioProcessor::openFileOutputStream(const SF_INFO& sfInfo) {
    closeStream(); // Ensure any existing stream is closed

    PaDeviceIndex outputDeviceIndex = Pa_GetDefaultOutputDevice();
    if (outputDeviceIndex == paNoDevice) {
        std::cerr << "Error: No default output device found for file playback." << std::endl;
        return false;
    }
    const PaDeviceInfo* outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);
    if (!outputDeviceInfo) {
        std::cerr << "Error getting output device info for file playback." << std::endl;
        return false;
    }

    // Use file's channel count, but verify against output device capabilities
    int requestedChannels = sfInfo.channels;
    if (requestedChannels > outputDeviceInfo->maxOutputChannels) {
        std::cerr << "Warning: Requested " << requestedChannels << " channels for file playback not supported by output device (Max: " 
                  << outputDeviceInfo->maxOutputChannels << "). Falling back." << std::endl;
        if (outputDeviceInfo->maxOutputChannels >= 2) requestedChannels = 2;
        else if (outputDeviceInfo->maxOutputChannels >= 1) requestedChannels = 1;
        else { /* Error */ return false; }
        std::cerr << "Using " << requestedChannels << " channels instead." << std::endl;
    }
    
    m_numChannels = requestedChannels; // Set the actual number of channels
    m_activeSampleRate = sfInfo.samplerate;

    PaStreamParameters outputParams;
    outputParams.device = outputDeviceIndex;
    outputParams.channelCount = m_numChannels; // Use the final channel count
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(
        &m_stream,
        nullptr,           // NO input parameters
        &outputParams,     // Output parameters based on file
        m_activeSampleRate,// Use file's sample rate
        m_framesPerBuffer,
        paClipOff,
        paCallback,
        this
    );

    if (err != paNoError) {
        std::cerr << "Error opening file output stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Error starting file output stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        return false;
    }

    m_audioData.resize(m_framesPerBuffer * m_numChannels, 0.0f);

    // Re-initialize FFT (needed for visualization, even if input isn't active)
    size_t fftSize = m_framesPerBuffer / 2 + 1;
    if (m_fftIn) fftwf_free(m_fftIn);
    if (m_fftOut) fftwf_free(m_fftOut);
    m_fftIn = (float*) fftwf_malloc(sizeof(float) * m_framesPerBuffer);
    m_fftOut = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fftSize);
    if (m_fftPlan) fftwf_destroy_plan(m_fftPlan);
    if (m_fftIn && m_fftOut) {
        m_fftPlan = fftwf_plan_dft_r2c_1d(m_framesPerBuffer, m_fftIn, m_fftOut, FFTW_ESTIMATE);
        if (!m_fftPlan) { /* error handling */ closeStream(); return false; }
    } else { /* error handling */ closeStream(); return false; }

    std::cout << "File output stream opened successfully:\n"
              << "- Output: " << outputDeviceInfo->name << "\n"
              << "- Channels: " << m_numChannels << "\n"
              << "- Sample Rate: " << m_activeSampleRate << "\n"
              << "- Buffer Size: " << m_framesPerBuffer << std::endl;

    return true;
}

bool AudioProcessor::initialize(int sampleRate, int framesPerBuffer, int numChannels) {
    m_sampleRate = sampleRate; // Store default sample rate
    m_framesPerBuffer = framesPerBuffer;
    m_numChannels = numChannels; // Store preferred channels
    m_activeSampleRate = sampleRate; // Set initial active rate

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Error initializing PortAudio: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    m_inputDevices.clear();
    int numDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo && deviceInfo->maxInputChannels > 0) {
            m_inputDevices.push_back(deviceInfo->name);
        }
    }

    m_currentDeviceIndex = Pa_GetDefaultInputDevice();
    if (m_currentDeviceIndex == paNoDevice) {
         std::cerr << "Error: No default input device found." << std::endl;
         return false; // Cannot proceed without a default input
    }

    // Initial stream opening for the default input device
    return openInputStream(m_currentDeviceIndex, Pa_GetDefaultOutputDevice(), m_activeSampleRate);
}

bool AudioProcessor::setInputDevice(int deviceIndex) {
    if (deviceIndex < 0 || deviceIndex >= Pa_GetDeviceCount()) {
        std::cerr << "Error: Invalid input device index." << std::endl;
        return false;
    }

    // If the device hasn't changed, do nothing
    if (deviceIndex == m_currentDeviceIndex && !m_isPlayingFile) {
        return true;
    }

    m_currentDeviceIndex = deviceIndex;
    m_isPlayingFile = false; 
    if (m_sndFile) { sf_close(m_sndFile); m_sndFile = nullptr; }

    const PaDeviceInfo* inputDeviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if (!inputDeviceInfo) { /* error handling */ return false; }
    int deviceSampleRate = static_cast<int>(inputDeviceInfo->defaultSampleRate);

    // Reopen the INPUT stream with the new input device and its default sample rate
    if (!openInputStream(deviceIndex, Pa_GetDefaultOutputDevice(), deviceSampleRate)) {
        std::cerr << "Failed to reopen input stream for new device." << std::endl;
        return false;
    }

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
        m_sndFile = nullptr; // Close previous file if any
    }

    m_sndFile = sf_open(filePath.c_str(), SFM_READ, &m_sfInfo);
    if (!m_sndFile) {
        std::cerr << "Error opening audio file: " << filePath << " : " << sf_strerror(NULL) << std::endl;
        return false;
    }

    // Open the dedicated FILE stream using the file's info
    if (!openFileOutputStream(m_sfInfo)) { 
        std::cerr << "Failed to open stream for audio file playback." << std::endl;
        sf_close(m_sndFile);
        m_sndFile = nullptr;
        // Optionally, try reopening the input stream? 
        // openInputStream(m_currentDeviceIndex, Pa_GetDefaultOutputDevice(), m_activeSampleRate);
        return false;
    }

    m_isPlayingFile = true;
    setFilePlayback(true);
    std::cout << "Loaded audio file: " << filePath 
              << " (Rate: " << m_sfInfo.samplerate 
              << ", Channels: " << m_sfInfo.channels << ")" << std::endl;
    return true;
}

int AudioProcessor::paCallback(const void* inputBuffer, void* outputBuffer,
                               unsigned long framesPerBuffer,
                               [[maybe_unused]] const PaStreamCallbackTimeInfo* timeInfo,
                               [[maybe_unused]] PaStreamCallbackFlags statusFlags,
                               void* userData) {
    AudioProcessor* processor = static_cast<AudioProcessor*>(userData);
    float* out = (float*)outputBuffer;
    // inputBuffer might be null if we opened an output-only stream for file playback
    const float* in = (const float*)inputBuffer; 
    unsigned long samplesToProcess = framesPerBuffer * processor->m_numChannels;

    // Ensure internal buffer matches stream config
    if (processor->m_audioData.size() != samplesToProcess) {
        processor->m_audioData.resize(samplesToProcess, 0.0f);
    }

    if (processor->m_isPlayingFile && processor->m_sndFile) {
        sf_count_t count = sf_read_float(processor->m_sndFile, processor->m_audioData.data(), samplesToProcess);
        if (static_cast<unsigned long>(count) < samplesToProcess) {
            // Fill remaining buffer with silence
            for (unsigned long i = count; i < samplesToProcess; ++i) {
                processor->m_audioData[i] = 0.0f;
            }
            sf_seek(processor->m_sndFile, 0, SEEK_SET);
        }
        // Copy data from internal buffer (filled from file) to output
        for (unsigned long i = 0; i < samplesToProcess; i++) {
            out[i] = processor->m_audioData[i];
        }
    } else if (in != nullptr) {
        // Copy live input data to internal buffer AND output buffer
        for (unsigned long i = 0; i < samplesToProcess; i++) {
            processor->m_audioData[i] = in[i];
            out[i] = in[i]; 
        }
    } else {
        // No input and no file playing (or output-only stream), output silence
        for (unsigned long i = 0; i < samplesToProcess; i++) {
            processor->m_audioData[i] = 0.0f;
            out[i] = 0.0f;
        }
    }

    // FFT Processing (should always happen based on m_audioData)
    if (processor->m_fftPlan && processor->m_fftIn && processor->m_fftOut) {
        // Prepare FFT input (use m_audioData)
        for (int i = 0; i < processor->m_framesPerBuffer; ++i) {
            size_t data_index = static_cast<size_t>(i) * processor->m_numChannels;
            // Use first channel or average if stereo
             if (processor->m_numChannels == 1) {
                 processor->m_fftIn[i] = (data_index < processor->m_audioData.size()) ? processor->m_audioData[data_index] : 0.0f;
             } else if (processor->m_numChannels == 2) {
                  size_t right_channel_index = data_index + 1;
                  float left = (data_index < processor->m_audioData.size()) ? processor->m_audioData[data_index] : 0.0f;
                  float right = (right_channel_index < processor->m_audioData.size()) ? processor->m_audioData[right_channel_index] : 0.0f;
                  processor->m_fftIn[i] = (left + right) / 2.0f; // Average stereo channels for FFT
             } else { // Handle other channel counts if necessary
                 processor->m_fftIn[i] = (data_index < processor->m_audioData.size()) ? processor->m_audioData[data_index] : 0.0f;
             }
        }
        fftwf_execute(processor->m_fftPlan);
        // Process FFT output to get band energies (as before)
        processor->processFFT(); // Encapsulate FFT processing logic
    }

    return paContinue;
}

// Encapsulate FFT processing logic into a separate function
void AudioProcessor::processFFT() {
    size_t fftSize = m_framesPerBuffer / 2 + 1;
    if (m_bandEnergies.size() != static_cast<size_t>(m_numBands)) {
        m_bandEnergies.resize(m_numBands, 0.0f);
        m_previousBandEnergies.resize(m_numBands, 0.0f);
    }

    for (int i = 0; i < m_numBands; i++) {
        float sum = 0;
        int start_k = static_cast<int>(pow(2, static_cast<float>(i) / m_numBands * log2(fftSize)));
        int end_k = static_cast<int>(pow(2, static_cast<float>(i + 1) / m_numBands * log2(fftSize)));
        start_k = std::max(1, std::min(start_k, (int)fftSize - 1));
        end_k = std::max(1, std::min(end_k, (int)fftSize));
        if (start_k >= end_k) continue;

        for (int k = start_k; k < end_k; k++) {
            float real = m_fftOut[k][0];
            float imag = m_fftOut[k][1];
            float magnitude = sqrt(real * real + imag * imag);
            sum += magnitude;
        }

        float currentEnergy = (end_k > start_k) ? (sum / (end_k - start_k)) : 0.0f;
        m_bandEnergies[i] = m_smoothingFactor * currentEnergy +
                            (1.0f - m_smoothingFactor) * m_previousBandEnergies[i];
        m_previousBandEnergies[i] = m_bandEnergies[i];
        m_bandEnergies[i] *= m_normalizationFactor;
    }
}

// Getter for playback state
bool AudioProcessor::isCurrentlyPlayingFile() const {
    return m_isPlayingFile && m_stream != nullptr; // Also check if stream is active
}

void AudioProcessor::setFilePlayback(bool play) {
    if (!m_stream) {
        std::cerr << "Warning: Cannot set playback state, stream is not open." << std::endl;
        return;
    }

    if (play && !m_isPlayingFile) {
        // Start playback
        PaError err = Pa_StartStream(m_stream);
        if (err != paNoError) {
            std::cerr << "Error starting stream for playback: " << Pa_GetErrorText(err) << std::endl;
        } else {
            m_isPlayingFile = true;
            std::cout << "Stream started for file playback." << std::endl;
        }
    } else if (!play && m_isPlayingFile) {
        // Stop playback
        PaError err = Pa_StopStream(m_stream);
        if (err != paNoError) {
            std::cerr << "Error stopping stream: " << Pa_GetErrorText(err) << std::endl;
        } else {
            m_isPlayingFile = false;
            // Optional: Seek back to beginning when pausing/stopping?
            if (m_sndFile) {
                sf_seek(m_sndFile, 0, SEEK_SET);
            }
            std::cout << "Stream stopped for file playback." << std::endl;
        }
    }
    // If play == m_isPlayingFile, do nothing (already in desired state)
}

void AudioProcessor::switchToInputDevice() {
    // This function specifically switches back to the last selected *input* device
    // after file playback has stopped.
    setInputDevice(m_currentDeviceIndex); 
    // setInputDevice already handles stopping file playback and reopening the stream.
}