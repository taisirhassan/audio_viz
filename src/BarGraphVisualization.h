#pragma once

#include "IVisualizationStyle.h"
#include <vector>
#include <cmath>
#include <GL/glew.h> // Include GLEW for GL types
#include "../include/shader.h" // Include Shader class definition
#include <memory> // For std::unique_ptr
#include <glm/glm.hpp>

class BarGraphVisualization : public IVisualizationStyle {
public:
    BarGraphVisualization();
    ~BarGraphVisualization() override; // Need custom destructor for cleanup

    void init() override; // Add init method
    void render(RenderParameters& params) override;
    void cleanup() override; // Add cleanup method
    void resize(int width, int height) override;

    // Implement the new interface method
    glm::mat4 getProjectionMatrix(int width, int height) const override;

    // Method to switch rendering mode
    void set3DMode(bool is3D);
    bool is3DMode() const { return m_is3D; } // Optional getter

private:
    // Helper to interpolate color (now moved to shader, can be removed from here later)
    // glm::vec3 interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);

    // Shaders
    std::unique_ptr<Shader> m_shader2D;
    std::unique_ptr<Shader> m_shader3D;

    // OpenGL objects
    GLuint m_vao2D = 0; // VAO for 2D quads
    GLuint m_vbo2D = 0; // VBO for 2D quads
    GLuint m_vao3D = 0; // VAO for 3D cubes
    GLuint m_vbo3D = 0; // VBO for 3D cubes
    GLuint m_tbo = 0;   // TBO for energies (shared)
    GLuint m_tboTexture = 0; // Texture for TBO (shared)
    static constexpr int TBO_TEXTURE_UNIT = 0; 

    // Mode flag
    bool m_is3D = true; // Default to 3D mode

    // Store both projection matrices
    glm::mat4 m_projectionOrtho;
    glm::mat4 m_projectionPerspective;

    // Helper to calculate orthographic projection
    void calculateOrthoProjection(int width, int height);
    // Helper to calculate perspective projection
    void calculatePerspectiveProjection(int width, int height);

    // Internal state for smoothing (if we decide to add it later)
    // std::vector<float> m_smoothedEnergies;
}; 