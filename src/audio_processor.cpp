#include "audio_processor.h"
#include <iostream>
#include <cmath>

AudioProcessor::AudioProcessor()
    : m_stream(nullptr), m_sndFile(nullptr), m_isPlayingFile(false),
      m_sampleRate(0), m_framesPerBuffer(0), m_numChannels(0), m_numBands(64),
      m_smoothingFactor(0.3f), m_normalizationFactor(1.0f) {}

AudioProcessor::~AudioProcessor() {
    if (m_stream) {
        Pa_CloseStream(m_stream);
    }
    if (m_sndFile) {
        sf_close(m_sndFile);
    }
    Pa_Terminate();
    fftwf_destroy_plan(m_fftPlan);
}

bool AudioProcessor::initialize(int sampleRate, int framesPerBuffer, int numChannels) {
    m_sampleRate = sampleRate;
    m_framesPerBuffer = framesPerBuffer;
    m_numChannels = numChannels;

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Get default input device
    PaDeviceIndex inputDevice = Pa_GetDefaultInputDevice();
    if (inputDevice == paNoDevice) {
        std::cerr << "PortAudio error: No default input device." << std::endl;
        return false;
    }

    // Get device info
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(inputDevice);
    if (!deviceInfo) {
        std::cerr << "PortAudio error: Unable to get device info." << std::endl;
        return false;
    }

    // Check if the device supports the requested number of channels
    if (deviceInfo->maxInputChannels < numChannels) {
        std::cerr << "Warning: Device does not support " << numChannels << " channels. "
                  << "Using " << deviceInfo->maxInputChannels << " channels instead." << std::endl;
        m_numChannels = deviceInfo->maxInputChannels;
    }

    // Open PortAudio stream
    PaStreamParameters inputParameters;
    inputParameters.device = inputDevice;
    inputParameters.channelCount = m_numChannels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&m_stream,
                        &inputParameters,
                        NULL,  // No output parameters
                        sampleRate,
                        framesPerBuffer,
                        paClipOff,  // We won't output out-of-range samples so don't bother clipping them
                        paCallback,
                        this);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Start PortAudio stream
    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Initialize audio data vectors
    m_audioData.resize(framesPerBuffer * m_numChannels);
    m_fftData.resize(framesPerBuffer);
    m_bandEnergies.resize(m_numBands);

    // Create FFTW plan
    m_fftPlan = fftwf_plan_r2r_1d(framesPerBuffer, m_audioData.data(), m_fftData.data(), FFTW_R2HC, FFTW_ESTIMATE);

    std::cout << "Audio processor initialized with " << m_numChannels << " channels." << std::endl;
    return true;
}

void AudioProcessor::processAudio() {
    // Execute FFT
    fftwf_execute(m_fftPlan);

    // Initialize previous band energies if needed
    if (m_previousBandEnergies.empty()) {
        m_previousBandEnergies.resize(m_numBands, 0.0f);
    }

    // Calculate band energies with improved frequency distribution
    for (int i = 0; i < m_numBands; i++) {
        float sum = 0;
        int start = (int)pow(2, (float)i / m_numBands * log2(m_framesPerBuffer / 2));
        int end = (int)pow(2, (float)(i + 1) / m_numBands * log2(m_framesPerBuffer / 2));
        
        // Use logarithmic frequency distribution for better bass response
        for (int j = start; j < end; j++) {
            float magnitude = sqrt(m_fftData[j] * m_fftData[j] + 
                                 m_fftData[m_framesPerBuffer - j] * m_fftData[m_framesPerBuffer - j]);
            // Apply frequency-dependent scaling
            float scale = 1.0f / (1.0f + j * 0.1f);
            sum += magnitude * scale;
        }
        
        // Apply smoothing
        float currentEnergy = sum / (end - start);
        m_bandEnergies[i] = m_smoothingFactor * currentEnergy + 
                          (1.0f - m_smoothingFactor) * m_previousBandEnergies[i];
        m_previousBandEnergies[i] = m_bandEnergies[i];
        
        // Apply normalization
        m_bandEnergies[i] *= m_normalizationFactor;
    }
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