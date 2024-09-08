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

private:
    void renderBarGraph();
    void renderCircular();
    void renderWave();
    glm::vec3 getColor(float t);

    AudioProcessor& m_audioProcessor;
    Shader m_shader;
    GLuint m_VAO, m_VBO;
    VisualizationStyle m_style;
    float m_rotationSpeed;
    float m_zoomLevel;
    int m_width;
    int m_height;

    glm::vec3 m_lowColor;
    glm::vec3 m_midColor;
    glm::vec3 m_highColor;
};
