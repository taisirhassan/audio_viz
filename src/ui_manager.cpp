#include <GL/glew.h> // Include GLEW first!
#include "ui_manager.h"
#include <glm/glm.hpp>
#include <string> // Ensure string is included if needed
#include <iostream>
#include <vector>
#include <imgui.h>
#include <nfd.h>

// Include GLFW header for the helper function
#if defined(_WIN32)
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
    #define GLFW_EXPOSE_NATIVE_COCOA
#else // Assume X11
    #define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h> // For native handles
#include <nfd_glfw3.h>

UIManager::UIManager(AudioProcessor& processor, Visualizer& visualizer)
    : m_audioProcessor(processor),
      m_visualizer(visualizer),
      m_currentDeviceIndex(processor.getCurrentDeviceIndex()) // Initialize with actual current device
{
    // Ensure initial style matches visualizer's default
    m_currentStyle = static_cast<int>(m_visualizer.getStyle());

    // Initialize UI color members from Visualizer defaults
    glm::vec3 low = m_visualizer.getLowColor();
    glm::vec3 mid = m_visualizer.getMidColor();
    glm::vec3 high = m_visualizer.getHighColor();
    m_uiLowColor[0] = low.x; m_uiLowColor[1] = low.y; m_uiLowColor[2] = low.z;
    m_uiMidColor[0] = mid.x; m_uiMidColor[1] = mid.y; m_uiMidColor[2] = mid.z;
    m_uiHighColor[0] = high.x; m_uiHighColor[1] = high.y; m_uiHighColor[2] = high.z;

    // Initialize NFD
    NFD_Init();
}

UIManager::~UIManager() {
    // Cleanup NFD
    NFD_Quit();
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
    
    // Use the UIManager's member variables for ColorEdit3
    // float lowColor[3] = { m_visualizer.getLowColor().x, m_visualizer.getLowColor().y, m_visualizer.getLowColor().z };
    // float midColor[3] = { m_visualizer.getMidColor().x, m_visualizer.getMidColor().y, m_visualizer.getMidColor().z };
    // float highColor[3] = { m_visualizer.getHighColor().x, m_visualizer.getHighColor().y, m_visualizer.getHighColor().z };
    
    bool colorChanged = false;
    // Pass UIManager members to ColorEdit3
    if (ImGui::ColorEdit3("Low Frequency", m_uiLowColor)) { colorChanged = true; }
    if (ImGui::ColorEdit3("Mid Frequency", m_uiMidColor)) { colorChanged = true; }
    if (ImGui::ColorEdit3("High Frequency", m_uiHighColor)) { colorChanged = true; }
    
    if (colorChanged) {
        // Update Visualizer when UI changes
        m_visualizer.updateColors(
            glm::vec3(m_uiLowColor[0], m_uiLowColor[1], m_uiLowColor[2]),
            glm::vec3(m_uiMidColor[0], m_uiMidColor[1], m_uiMidColor[2]),
            glm::vec3(m_uiHighColor[0], m_uiHighColor[1], m_uiHighColor[2])
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
        if (ImGui::Button("Open Audio File")) {
            nfdu8char_t *outPath = NULL;
            nfdu8filteritem_t filterItem[1] = { { "Audio Files", "wav,mp3,ogg,aiff,flac,m4a" } };
            
            // Prepare arguments for NFD
            nfdopendialognargs_t args = {0};
            args.filterList = filterItem;
            args.filterCount = 1;
            
            // Use the helper function from nfd_glfw3.h to get the native window handle
            bool handleOk = NFD_GetNativeWindowFromGLFWWindow(window, &args.parentWindow);
            if (!handleOk) {
                std::cerr << "Error getting native window handle" << std::endl;
            }
            
            nfdresult_t result = NFD_OpenDialogN_With(&outPath, &args);
            
            if (result == NFD_OKAY) {
                std::cout << "NFD Success! Path: " << outPath << std::endl;
                std::string pathStr = outPath; // Convert to std::string
                if (!pathStr.empty()) {
                    m_audioProcessor.loadAudioFile(pathStr);
                }
                NFD_FreePathN(outPath); // Remember to free the path!
            } else if (result == NFD_CANCEL) {
                std::cout << "User pressed cancel." << std::endl;
            } else {
                printf("NFD Error: %s\n", NFD_GetError());
            }
        }

        ImGui::SameLine();
        // Add Play/Pause/Stop buttons (using AudioProcessor state)
        if (m_audioProcessor.hasLoadedFile()) {
            if (m_audioProcessor.isCurrentlyPlayingFile()) {
                if (ImGui::Button("Pause")) {
                    m_audioProcessor.setFilePlayback(false);
                }
            } else {
                if (ImGui::Button("Play")) {
                    m_audioProcessor.setFilePlayback(true);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop File & Use Mic")) {
                m_audioProcessor.switchToInputDevice(); // This now also stops playback
            }
        }
    }

    ImGui::End();
} 