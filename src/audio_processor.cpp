#include "audio_processor.h"
#include <iostream>
#include <cmath>

AudioProcessor::AudioProcessor()
    : m_stream(nullptr), m_sndFile(nullptr), m_isPlayingFile(false),
      m_sampleRate(0), m_framesPerBuffer(0), m_numChannels(0), m_numBands(64) {}

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

    // Calculate band energies
    for (int i = 0; i < m_numBands; i++) {
        float sum = 0;
        int start = (int)pow(2, (float)i / m_numBands * log2(m_framesPerBuffer / 2));
        int end = (int)pow(2, (float)(i + 1) / m_numBands * log2(m_framesPerBuffer / 2));
        for (int j = start; j < end; j++) {
            sum += sqrt(m_fftData[j] * m_fftData[j] + m_fftData[m_framesPerBuffer - j] * m_fftData[m_framesPerBuffer - j]);
        }
        m_bandEnergies[i] = sum / (end - start);
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

int AudioProcessor::paCallback(const void* inputBuffer, void* outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userData) {
    AudioProcessor* processor = static_cast<AudioProcessor*>(userData);
    float* in = (float*)inputBuffer;
    
    if (processor->m_isPlayingFile && processor->m_sndFile) {
        sf_count_t count = sf_read_float(processor->m_sndFile, processor->m_audioData.data(), framesPerBuffer * processor->m_numChannels);
        if (count < framesPerBuffer * processor->m_numChannels) {
            sf_seek(processor->m_sndFile, 0, SEEK_SET);  // Loop back to the beginning of the file
        }
    } else {
        for (unsigned long i = 0; i < framesPerBuffer * processor->m_numChannels; i++) {
            processor->m_audioData[i] = in[i];
        }
    }

    return paContinue;
}