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

    // Data for smoothing visualization changes
    float m_smoothingFactorViz = 0.2f; // Separate from audio processing smoothing

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
    float m_rotationSpeed = 0.0f; // Default value
    float m_zoomLevel = 1.0f;     // Default value
    int m_width;
    int m_height;
    
    // Projection and View matrices (calculated here, passed to styles)
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;

    // OpenGL objects removed (managed by individual styles now)
    // GLuint m_VAO;
    // GLuint m_VBO;
    // GLuint m_shaderProgram;
    // Shader m_shader;

    // Camera state for orbit control (Some parts removed for auto-rotation)
    glm::vec3 m_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // Look at origin
    glm::vec3 m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);     // World up
    float m_cameraRadius = 3.0f; // Initial distance from target (still used)
    float m_cameraFov = 45.0f; // Store FOV for projection (still used)

    // Customizable colors
    glm::vec3 m_customLowColor = glm::vec3(0.0f, 0.0f, 1.0f); // Default blue
    glm::vec3 m_customMidColor = glm::vec3(0.0f, 1.0f, 0.0f); // Default green
    glm::vec3 m_customHighColor = glm::vec3(1.0f, 0.0f, 0.0f); // Default red

    // Initialization flag
    bool m_isInitialized = false;
};
