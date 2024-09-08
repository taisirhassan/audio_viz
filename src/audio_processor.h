#pragma once

#include <portaudio.h>
#include <sndfile.h>
#include <fftw3.h>
#include <vector>
#include <string>

class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();

    bool initialize(int sampleRate, int framesPerBuffer, int numChannels);
    void processAudio();
    bool loadAudioFile(const std::string& filePath);
    void toggleAudioSource();
    const std::vector<float>& getBandEnergies() const { return m_bandEnergies; }
    const std::vector<float>& getAudioData() const { return m_audioData; }

private:
    static int paCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    PaStream* m_stream;
    SNDFILE* m_sndFile;
    SF_INFO m_sfInfo;
    std::vector<float> m_audioData;
    std::vector<float> m_fftData;
    std::vector<float> m_bandEnergies;
    fftwf_plan m_fftPlan;
    bool m_isPlayingFile;

    int m_sampleRate;
    int m_framesPerBuffer;
    int m_numChannels;
    int m_numBands;
};