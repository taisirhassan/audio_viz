#include <GL/glew.h> // Include GLEW first!
#include "ui_manager.h"
#include <glm/glm.hpp>
#include <string> // Ensure string is included if needed
#include <iostream>
#include <vector>
#include <imgui.h>

// Declare the platform-specific file dialog function
// This might eventually move into a platform abstraction layer
std::string openFileDialog();

UIManager::UIManager(AudioProcessor& processor, Visualizer& visualizer)
    : m_audioProcessor(processor),
      m_visualizer(visualizer),
      m_currentDeviceIndex(processor.getCurrentDeviceIndex()) // Initialize with actual current device
{
    // Ensure initial style matches visualizer's default
    m_currentStyle = static_cast<int>(m_visualizer.getStyle());
}

void UIManager::render(GLFWwindow* window) {
    // Note: The 'window' parameter is currently unused in the ImGui logic
    // but kept for potential future use (e.g., interactions specific to the window)
    (void)window; // Explicitly mark as unused to suppress warnings

    // Set window position and size (same as before)
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

    ImGui::Begin("Audio Visualizer Controls", nullptr, 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoCollapse);

    // Audio device selection
    if (ImGui::CollapsingHeader("Audio Device", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& devices = m_audioProcessor.getInputDevices();
        // Use member variable m_currentDeviceIndex
        if (!devices.empty()) { // Add check for empty devices list
             // Ensure m_currentDeviceIndex is valid
            if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= static_cast<int>(devices.size())) {
                m_currentDeviceIndex = m_audioProcessor.getCurrentDeviceIndex(); // Reset if invalid
                 if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= static_cast<int>(devices.size())) {
                     m_currentDeviceIndex = 0; // Default to 0 if still invalid
                 }
            }

            const char* currentDeviceName = (m_currentDeviceIndex >= 0 && m_currentDeviceIndex < static_cast<int>(devices.size())) ? devices[m_currentDeviceIndex].c_str() : "No Device";
            if (ImGui::BeginCombo("Input Device", currentDeviceName)) {
                for (size_t i = 0; i < devices.size(); i++) {
                    bool isSelected = (m_currentDeviceIndex == static_cast<int>(i));
                    if (ImGui::Selectable(devices[i].c_str(), isSelected)) {
                        if (m_currentDeviceIndex != static_cast<int>(i)) { // Only switch if changed
                           m_currentDeviceIndex = static_cast<int>(i);
                           m_audioProcessor.setInputDevice(m_currentDeviceIndex);
                        }
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else {
             ImGui::Text("No input devices found.");
        }
    }

    // Visualization controls
    ImGui::Text("Visualization");
    ImGui::Separator();
    
    // Use member variable m_currentStyle
    const char* styles[] = { "Bar Graph", "Circular", "Wave" };
    if (ImGui::Combo("Style", &m_currentStyle, styles, IM_ARRAYSIZE(styles))) {
        m_visualizer.updateSettings(static_cast<VisualizationStyle>(m_currentStyle),
                                   m_visualizer.getRotationSpeed(),
                                   m_visualizer.getZoomLevel());
    }

    ImGui::Spacing();
    float rotationSpeed = m_visualizer.getRotationSpeed();
    if (ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 5.0f)) {
        m_visualizer.updateSettings(static_cast<VisualizationStyle>(m_currentStyle),
                               rotationSpeed,
                               m_visualizer.getZoomLevel());
    }

    float zoomLevel = m_visualizer.getZoomLevel();
    if (ImGui::SliderFloat("Zoom Level", &zoomLevel, 0.1f, 2.0f)) {
        m_visualizer.updateSettings(static_cast<VisualizationStyle>(m_currentStyle),
                               rotationSpeed,
                               zoomLevel);
    }

    // Colors Section (same as before, using m_visualizer directly)
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Colors");
    ImGui::Spacing();
    
    float lowColor[3] = { m_visualizer.getLowColor().x, m_visualizer.getLowColor().y, m_visualizer.getLowColor().z };
    float midColor[3] = { m_visualizer.getMidColor().x, m_visualizer.getMidColor().y, m_visualizer.getMidColor().z };
    float highColor[3] = { m_visualizer.getHighColor().x, m_visualizer.getHighColor().y, m_visualizer.getHighColor().z };
    
    bool colorChanged = false;
    if (ImGui::ColorEdit3("Low Frequency", lowColor)) { colorChanged = true; }
    if (ImGui::ColorEdit3("Mid Frequency", midColor)) { colorChanged = true; }
    if (ImGui::ColorEdit3("High Frequency", highColor)) { colorChanged = true; }
    
    if (colorChanged) {
        m_visualizer.updateColors(
            glm::vec3(lowColor[0], lowColor[1], lowColor[2]),
            glm::vec3(midColor[0], midColor[1], midColor[2]),
            glm::vec3(highColor[0], highColor[1], highColor[2])
        );
    }

    // Audio processing parameters (same as before, using m_audioProcessor directly)
    if (ImGui::CollapsingHeader("Audio Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
        float smoothing = m_audioProcessor.getSmoothingFactor();
        if (ImGui::SliderFloat("Smoothing", &smoothing, 0.0f, 0.95f)) {
            m_audioProcessor.setSmoothingFactor(smoothing);
        }
        ImGui::SameLine(); 
        if (ImGui::Button("Reset##Smoothing")) { // Unique ID for Reset
            m_audioProcessor.setSmoothingFactor(0.3f);
        }

        float normalization = m_audioProcessor.getNormalizationFactor();
        if (ImGui::SliderFloat("Gain", &normalization, 0.1f, 20.0f)) {
            m_audioProcessor.setNormalizationFactor(normalization);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset##Gain")) { // Unique ID for Reset
            m_audioProcessor.setNormalizationFactor(5.0f);
        }
    }

    // File playback controls (using m_audioProcessor directly)
    if (ImGui::CollapsingHeader("File Playback", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool fileLoaded = m_audioProcessor.hasLoadedFile();

        if (ImGui::Button("Open Audio File")) {
            std::string filePath = openFileDialog();
            if (!filePath.empty()) {
                m_audioProcessor.loadAudioFile(filePath);
                fileLoaded = m_audioProcessor.hasLoadedFile(); // Re-check status
            }
        }
        
        if (fileLoaded) {
            bool isCurrentlyPlaying = m_audioProcessor.isCurrentlyPlayingFile();
            ImGui::SameLine();
            if (ImGui::Button(isCurrentlyPlaying ? "Pause" : "Play")) {
                m_audioProcessor.setFilePlayback(!isCurrentlyPlaying);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop File & Use Mic")) {
                m_audioProcessor.switchToInputDevice();
            }
        }
    }

    ImGui::End();
} 