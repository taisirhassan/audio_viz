#pragma once

#include <GL/glew.h>

// Include other headers AFTER GLEW
#include "visualizer.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "audio_processor.h"
#include <string>

// Forward declarations might not be strictly needed now but good practice
class AudioProcessor; 
class Visualizer;

class UIManager {
public:
    UIManager(AudioProcessor& audioProcessor, Visualizer& visualizer);
    ~UIManager();

    void render(GLFWwindow* window);

private:
    void loadSettings(); // Added load function

    AudioProcessor& m_audioProcessor;
    Visualizer& m_visualizer;

    // UI state
    int m_currentDeviceIndex = 0;
    int m_selectedDeviceIndex = 0;
    int m_selectedStyleIndex = 0;
    float m_rotationSpeed = 1.0f;
    float m_zoomLevel = 1.0f;
    glm::vec3 m_lowColor = {0.0f, 0.0f, 1.0f}; // Default blue
    glm::vec3 m_midColor = {0.0f, 1.0f, 0.0f}; // Default green
    glm::vec3 m_highColor = {1.0f, 0.0f, 0.0f}; // Default red

    // C-style arrays for ImGui ColorEdit (if needed, but prefer using glm::vec3 directly)
    // float m_uiLowColor[3] = {0.0f, 0.0f, 1.0f};
    // float m_uiMidColor[3] = {0.0f, 1.0f, 0.0f};
    // float m_uiHighColor[3] = {1.0f, 0.0f, 0.0f};

    // File dialog state
    bool m_fileDialogRequested = false;
}; 