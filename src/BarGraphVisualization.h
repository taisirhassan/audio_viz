#pragma once

#include "IVisualizationStyle.h"
#include <vector>
#include <cmath>
#include <GL/glew.h> // Include GLEW for GL types
#include "../include/shader.h" // Include Shader class definition
#include <memory> // For std::unique_ptr

class BarGraphVisualization : public IVisualizationStyle {
public:
    BarGraphVisualization();
    ~BarGraphVisualization() override; // Need custom destructor for cleanup

    void init() override; // Add init method
    void render(RenderParameters& params) override;
    void cleanup() override; // Add cleanup method

private:
    // Helper to interpolate color (now moved to shader, can be removed from here later)
    // glm::vec3 interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);

    // OpenGL objects
    std::unique_ptr<Shader> m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0; // For the base quad
    GLuint m_tbo = 0; // Texture Buffer Object for energies
    GLuint m_tboTexture = 0; // Texture ID for the TBO
    static constexpr int TBO_TEXTURE_UNIT = 0; // Assign a texture unit for the TBO

    // Internal state for smoothing (if we decide to add it later)
    // std::vector<float> m_smoothedEnergies;
}; 