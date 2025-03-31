#pragma once

#include "IVisualizationStyle.h"
#include <vector> // Include vector for m_smoothedEnergies member
#define _USE_MATH_DEFINES // For M_PI
#include <cmath>
#include <GL/glew.h>           // Include GLEW for GL types
#include "../include/shader.h" // Include Shader class definition
#include <memory>              // For std::unique_ptr
#include <glm/glm.hpp>         // Include glm for matrix operations

class CircularVisualization : public IVisualizationStyle {
public:
    CircularVisualization(); // Constructor to initialize smoothed data size
    ~CircularVisualization() override; // Need custom destructor for cleanup

    void init() override; // Add init method
    void render(RenderParameters& params) override;
    void cleanup() override; // Add cleanup method
    void resize(int width, int height) override;

    // Implement the new interface method
    glm::mat4 getProjectionMatrix(int width, int height) const override;

    // Optional overrides if needed for settings/colors
    // void updateSettings(float rotationSpeed, float zoomLevel) override; 
    // void updateColors(const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high) override;

private:
    // Helper to interpolate color (now moved to shader)
    // glm::vec3 interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);

    // Internal state for smoothing (needs to persist between frames)
    std::vector<float> m_smoothedEnergies;

    // OpenGL objects
    std::unique_ptr<Shader> m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0; // For the base quad (same as bar graph)
    GLuint m_tbo = 0; // Texture Buffer Object for smoothed energies
    GLuint m_tboTexture = 0; // Texture ID for the TBO
    static constexpr int TBO_TEXTURE_UNIT = 1; // Use a different texture unit than BarGraph

    // Store orthographic projection matrix
    glm::mat4 m_projectionOrtho;

    // Helper to calculate orthographic projection
    void calculateOrthoProjection(int width, int height);
}; 