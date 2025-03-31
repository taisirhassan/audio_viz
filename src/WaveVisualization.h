#pragma once

#include "IVisualizationStyle.h"
#include <cmath>
#include <GL/glew.h>           // Include GLEW for GL types
#include "../include/shader.h" // Include Shader class definition
#include <memory>              // For std::unique_ptr
#include <vector>              // Need vector for include consistency

class WaveVisualization : public IVisualizationStyle {
public:
    WaveVisualization();
    ~WaveVisualization() override; // Need custom destructor for cleanup

    void init() override; // Add init method
    void render(RenderParameters& params) override;
    void cleanup() override; // Add cleanup method

private:
    // Helper to interpolate color (now moved to shader)
    // glm::vec3 interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);

    // OpenGL objects
    std::unique_ptr<Shader> m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0; // For the base line segment (just two points)
    GLuint m_tbo = 0; // Texture Buffer Object for raw audio data
    GLuint m_tboTexture = 0; // Texture ID for the TBO
    static constexpr int TBO_TEXTURE_UNIT = 2; // Use a different texture unit
}; 