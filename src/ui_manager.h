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
    UIManager(AudioProcessor& processor, Visualizer& visualizer);
    ~UIManager();

    void render(GLFWwindow* window);

private:
    AudioProcessor& m_audioProcessor;
    Visualizer& m_visualizer;

    // Potentially move UI state variables here if needed (e.g., current device index)
    int m_currentDeviceIndex = -1; // Initialize to -1 or get from processor
    int m_currentStyle = 0;     // Example
    // Add members to hold UI color state
    float m_uiLowColor[3];
    float m_uiMidColor[3];
    float m_uiHighColor[3];
}; 