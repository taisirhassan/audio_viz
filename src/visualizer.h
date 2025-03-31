#pragma once

// GLEW must be included before any other OpenGL headers
#include <GL/glew.h>

// Other OpenGL-related headers
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Standard library and project headers
#include <vector>
#include <memory>
#include "shader.h"
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

    bool initialize(int width, int height);
    void render();
    void updateSettings(VisualizationStyle style, float rotationSpeed, float zoomLevel);
    void updateColors(const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);
    void resize(int width, int height);

    // Getters for UI
    float getRotationSpeed() const { return m_rotationSpeed; }
    float getZoomLevel() const { return m_zoomLevel; }
    VisualizationStyle getStyle() const { return m_style; }
    glm::vec3 getLowColor() const { return m_customLowColor; }
    glm::vec3 getMidColor() const { return m_customMidColor; }
    glm::vec3 getHighColor() const { return m_customHighColor; }

    // Data for smoothing visualization changes
    float m_smoothingFactorViz = 0.2f; // Separate from audio processing smoothing

private:
    // Remove old render methods
    // void renderBarGraph();
    // void renderCircular();
    // void renderWave();
    // Remove old color helpers if they are now inside the styles
    // glm::vec3 getColor(float t);
    // glm::vec3 interpolateColor(float hue, float energy);

    // Add pointer to the current style object
    std::unique_ptr<IVisualizationStyle> m_currentStylePtr;

    AudioProcessor& m_audioProcessor;
    VisualizationStyle m_style;
    float m_rotationSpeed;
    float m_zoomLevel;
    int m_width;
    int m_height;
    
    // OpenGL objects
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_shaderProgram;
    Shader m_shader;

    // Customizable colors
    glm::vec3 m_customLowColor;
    glm::vec3 m_customMidColor;
    glm::vec3 m_customHighColor;
};
