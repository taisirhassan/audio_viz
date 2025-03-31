// Define before *any* includes if using experimental GLM headers anywhere
#define GLM_ENABLE_EXPERIMENTAL

#pragma once

// GLEW must be included before any other OpenGL headers
#include <GL/glew.h>

// Other OpenGL-related headers
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp> // For debugging matrix/vector output

// Standard library and project headers
#include <vector>
#include <memory>
#include <map> // To store style instances
// #include "shader.h" // Shader managed by styles now
#include "audio_processor.h"
#include "IVisualizationStyle.h"
#include <unordered_map>
#include <string>
#include <array>

enum class VisualizationStyle {
    BAR_GRAPH,
    CIRCULAR,
    WAVE
};

class Visualizer {
public:
    Visualizer(AudioProcessor& audioProcessor);
    ~Visualizer();

    // Initialize now simply sets initial style
    bool initialize(int width, int height, VisualizationStyle initialStyle);
    void render();
    // Update settings now handles changing the style object if needed
    void updateSettings(VisualizationStyle style, float rotationSpeed, float zoomLevel);
    void updateColors(const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);
    void resize(int width, int height);

    // Specific control for Bar Graph mode
    void setBarGraph3DMode(bool is3D);

    // Camera control handlers (to be called by Application callbacks)
    void handleMouseButton(int button, int action, int mods);
    void handleMouseMove(double xpos, double ypos);
    void handleMouseScroll(double xoffset, double yoffset); // Add scroll for zoom/radius

    // Getters for UI
    float getRotationSpeed() const { return m_rotationSpeed; }
    float getZoomLevel() const { return m_zoomLevel; }
    VisualizationStyle getStyle() const { return m_currentStyleEnum; } // Return the enum
    glm::vec3 getLowColor() const { return m_customLowColor; }
    glm::vec3 getMidColor() const { return m_customMidColor; }
    glm::vec3 getHighColor() const { return m_customHighColor; }
    IVisualizationStyle* getCurrentStylePtr() const { return m_currentStylePtr; }

    // Data for smoothing visualization changes
    // float m_smoothingFactorViz = 0.2f; // Removed, let styles handle smoothing

private:
    // Method to change the active visualization style
    void setStyle(VisualizationStyle newStyle);

    // Remove old render methods
    // ... (already removed)
    // Remove old color helpers
    // ... (already removed)

    AudioProcessor& m_audioProcessor;
    
    // Store all styles, but only one is active
    std::map<VisualizationStyle, std::unique_ptr<IVisualizationStyle>> m_visualizationStyles;
    IVisualizationStyle* m_currentStylePtr = nullptr; // Raw pointer to the active style
    VisualizationStyle m_currentStyleEnum;

    // Settings
    float m_rotationSpeed = 1.0f;
    float m_zoomLevel = 1.0f;
    int m_width;
    int m_height;
    
    // Projection and View matrices (calculated here, passed to styles)
    glm::mat4 m_view;             // View matrix (camera position/orientation)
    // glm::mat4 m_projection;    // REMOVED - Projection is now per-style
    glm::vec3 m_cameraPos   = glm::vec3(0.0f, 0.0f, 5.0f); // Initial camera position
    glm::vec3 m_cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
    float m_yaw   = -90.0f; // Initial yaw
    float m_pitch = 0.0f;  // Initial pitch

    // Customizable colors
    glm::vec3 m_customLowColor = glm::vec3(0.0f, 0.0f, 1.0f); // Default blue
    glm::vec3 m_customMidColor = glm::vec3(0.0f, 1.0f, 0.0f); // Default green
    glm::vec3 m_customHighColor = glm::vec3(1.0f, 0.0f, 0.0f); // Default red

    // Initialization flag
    bool m_isInitialized = false;

    // Helper to update camera view matrix
    void updateCameraView();

    // Test triangle rendering for debugging
    // void drawTestTriangle(); // REMOVED
};
