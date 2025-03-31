#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "shader.h"
#include "audio_processor.h"

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

    // Getters for UI
    float getRotationSpeed() const { return m_rotationSpeed; }
    float getZoomLevel() const { return m_zoomLevel; }
    VisualizationStyle getStyle() const { return m_style; }

private:
    void renderBarGraph();
    void renderCircular();
    void renderWave();
    glm::vec3 getColor(float t);
    glm::vec3 interpolateColor(float hue, float energy);

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

    // Colors
    glm::vec3 m_lowColor;
    glm::vec3 m_midColor;
    glm::vec3 m_highColor;
};
