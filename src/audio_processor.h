#pragma once

#include <portaudio.h>
#include <sndfile.h>
#include <fftw3.h>
#include <vector>
#include <string>

struct AudioDevice {
    PaDeviceIndex index;
    std::string name;
    int maxInputChannels;
    int maxOutputChannels;
    double defaultSampleRate;
};

class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();

    bool initialize(int sampleRate = 44100, int framesPerBuffer = 1024, int numChannels = 2);
    void processAudio();
    bool loadAudioFile(const std::string& filePath);
    void toggleAudioSource();
    
    // Device management
    const std::vector<AudioDevice>& getAvailableDevices() const { return m_availableDevices; }
    int getCurrentDeviceIndex() const { return m_currentDeviceIndex; }
    bool switchToDevice(int deviceIndex);
    
    // Getters for visualization
    const std::vector<float>& getBandEnergies() const { return m_bandEnergies; }
    const std::vector<float>& getAudioData() const { return m_audioData; }
    float getSmoothingFactor() const { return m_smoothingFactor; }
    float getNormalizationFactor() const { return m_normalizationFactor; }
    
    // Setters for parameters
    void setSmoothingFactor(float value) { m_smoothingFactor = value; }
    void setNormalizationFactor(float value) { m_normalizationFactor = value; }

    // File playback
    bool hasLoadedFile() const { return m_sndFile != nullptr; }
    void setFilePlayback(bool play);
    bool isCurrentlyPlayingFile() const;

    // Device management
    const std::vector<std::string>& getInputDevices() const { return m_inputDevices; }
    bool setInputDevice(int deviceIndex);
    void switchToInputDevice();

private:
    static int paCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData);

    void refreshDeviceList();
    bool openInputStream(PaDeviceIndex inputDeviceIndex, PaDeviceIndex outputDeviceIndex, int sampleRate);
    bool openFileOutputStream(const SF_INFO& sfInfo);
    void closeStream();

    // Audio stream and processing
    PaStream* m_stream;
    SNDFILE* m_sndFile;
    SF_INFO m_sfInfo;
    bool m_isPlayingFile;
    
    // Audio parameters
    int m_sampleRate;
    int m_framesPerBuffer;
    int m_numChannels;
    int m_numBands;
    
    // Device management
    std::vector<AudioDevice> m_availableDevices;
    int m_currentDeviceIndex;
    int m_activeSampleRate;
    
    // Processing parameters
    float m_smoothingFactor;
    float m_normalizationFactor;
    
    // Audio data
    std::vector<float> m_audioData;
    std::vector<float> m_fftData;
    std::vector<float> m_bandEnergies;
    std::vector<float> m_previousBandEnergies;
    fftwf_plan m_fftPlan;
    float* m_fftIn;          // FFT input buffer
    fftwf_complex* m_fftOut; // FFT output buffer

    // File playback
    std::vector<std::string> m_inputDevices;
    SNDFILE* m_audioFile;

    void processFFT();
};