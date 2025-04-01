#include <GL/glew.h> // Include GLEW first!
#include "ui_manager.h"
#include <glm/glm.hpp>
#include <string> // Ensure string is included if needed
#include <iostream>
#include <vector>
#include <imgui.h>
#include <nfd.h>
#include "BarGraphVisualization.h"

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
      m_currentDeviceIndex(processor.getCurrentDeviceIndex()),
      m_selectedDeviceIndex(0),
      m_selectedStyleIndex(0),
      m_rotationSpeed(1.0f),
      m_zoomLevel(1.0f),
      m_lowColor(0.0f, 0.0f, 1.0f),
      m_midColor(0.0f, 1.0f, 0.0f),
      m_highColor(1.0f, 0.0f, 0.0f)
{
    // Initialize NFD
    NFD_Init();
}

UIManager::~UIManager() {
    // Cleanup NFD
    NFD_Quit();
    std::cout << "UIManager destructed." << std::endl;
}

void UIManager::loadSettings() {
    // For now, just initialize with defaults
    m_visualizer.setStyle(static_cast<VisualizationStyle>(m_selectedStyleIndex));
    m_visualizer.setColors(m_lowColor, m_midColor, m_highColor);
    m_visualizer.setRotationSpeed(m_rotationSpeed);
    m_visualizer.setZoomLevel(m_zoomLevel);
}

void UIManager::render(GLFWwindow* window) {
    // Set window position and size
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

    ImGui::Begin("Audio Visualizer Controls", nullptr, 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoCollapse);

    // Audio device selection
    if (ImGui::CollapsingHeader("Audio Device", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& devices = m_audioProcessor.getInputDevices();
        if (!devices.empty()) {
            if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= static_cast<int>(devices.size())) {
                m_currentDeviceIndex = m_audioProcessor.getCurrentDeviceIndex();
                if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= static_cast<int>(devices.size())) {
                    m_currentDeviceIndex = 0;
                }
            }

            const char* currentDeviceName = (m_currentDeviceIndex >= 0 && m_currentDeviceIndex < static_cast<int>(devices.size())) 
                ? devices[m_currentDeviceIndex].c_str() 
                : "No Device";

            if (ImGui::BeginCombo("Input Device", currentDeviceName)) {
                for (size_t i = 0; i < devices.size(); i++) {
                    bool isSelected = (m_currentDeviceIndex == static_cast<int>(i));
                    if (ImGui::Selectable(devices[i].c_str(), isSelected)) {
                        if (m_currentDeviceIndex != static_cast<int>(i)) {
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
    
    const char* styles[] = { "Bar Graph", "Circular", "Wave" };
    if (ImGui::Combo("Style", &m_selectedStyleIndex, styles, IM_ARRAYSIZE(styles))) {
        m_visualizer.setStyle(static_cast<VisualizationStyle>(m_selectedStyleIndex));
        std::cout << "UI: Switched style to " << styles[m_selectedStyleIndex] << std::endl;
    }

    // If the current style is BarGraph, show 3D option with better visibility
    auto currentStyle = m_visualizer.getStyle();
    if (currentStyle == VisualizationStyle::BAR_GRAPH) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255)); // Yellow text for emphasis
        ImGui::Separator();
        ImGui::Text("Bar Graph Mode:");
        ImGui::SameLine();
        
        auto barGraph = dynamic_cast<BarGraphVisualization*>(m_visualizer.getCurrentStylePtr());
        if (barGraph) {
            bool is3D = barGraph->is3DMode();
            if (ImGui::Checkbox("Enable 3D Mode", &is3D)) {
                barGraph->set3DMode(is3D);
            }
        }
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    ImGui::Spacing();
    if (ImGui::SliderFloat("Rotation Speed", &m_rotationSpeed, 0.0f, 5.0f)) {
        m_visualizer.setRotationSpeed(m_rotationSpeed);
    }

    if (ImGui::SliderFloat("Zoom Level", &m_zoomLevel, 0.1f, 2.0f)) {
        m_visualizer.setZoomLevel(m_zoomLevel);
    }

    // Colors Section
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Colors");
    ImGui::Spacing();
    
    bool colorChanged = false;
    if (ImGui::ColorEdit3("Low Frequency", &m_lowColor[0])) { colorChanged = true; }
    if (ImGui::ColorEdit3("Mid Frequency", &m_midColor[0])) { colorChanged = true; }
    if (ImGui::ColorEdit3("High Frequency", &m_highColor[0])) { colorChanged = true; }
    
    if (colorChanged) {
        m_visualizer.setColors(m_lowColor, m_midColor, m_highColor);
    }

    // Audio processing parameters
    if (ImGui::CollapsingHeader("Audio Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
        float smoothing = m_audioProcessor.getSmoothingFactor();
        if (ImGui::SliderFloat("Smoothing", &smoothing, 0.0f, 0.95f)) {
            m_audioProcessor.setSmoothingFactor(smoothing);
        }
        ImGui::SameLine(); 
        if (ImGui::Button("Reset##Smoothing")) {
            m_audioProcessor.setSmoothingFactor(0.3f);
        }

        float normalization = m_audioProcessor.getNormalizationFactor();
        if (ImGui::SliderFloat("Gain", &normalization, 0.1f, 20.0f)) {
            m_audioProcessor.setNormalizationFactor(normalization);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset##Gain")) {
            m_audioProcessor.setNormalizationFactor(5.0f);
        }
    }

    // File playback controls
    if (ImGui::CollapsingHeader("File Playback", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Open Audio File")) {
            nfdu8char_t *outPath = NULL;
            nfdu8filteritem_t filterItem[1] = { { "Audio Files", "wav,mp3,ogg,aiff,flac,m4a" } };
            
            nfdopendialognargs_t args = {0};
            args.filterList = filterItem;
            args.filterCount = 1;
            
            bool handleOk = NFD_GetNativeWindowFromGLFWWindow(window, &args.parentWindow);
            if (!handleOk) {
                std::cerr << "Error getting native window handle" << std::endl;
            }
            
            nfdresult_t result = NFD_OpenDialogN_With(&outPath, &args);
            
            if (result == NFD_OKAY) {
                std::cout << "NFD Success! Path: " << outPath << std::endl;
                std::string pathStr = outPath;
                if (!pathStr.empty()) {
                    m_audioProcessor.loadAudioFile(pathStr);
                }
                NFD_FreePathN(outPath);
            } else if (result == NFD_CANCEL) {
                std::cout << "User pressed cancel." << std::endl;
            } else {
                printf("NFD Error: %s\n", NFD_GetError());
            }
        }

        ImGui::SameLine();
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
                m_audioProcessor.switchToInputDevice();
            }
        }
    }

    ImGui::End();
} 