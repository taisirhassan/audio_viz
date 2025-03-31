#pragma once

#include <vector>
#include <glm/glm.hpp> // For matrices and vectors
#include <GL/glew.h>    // Include for GLuint

// Forward declarations if needed
// class AudioProcessor; // No longer needed here

// Struct to pass render parameters efficiently
struct RenderParameters {
    const std::vector<float>& bandEnergies;
    const std::vector<float>& audioData; // For Wave
    // const std::vector<float>& smoothedEnergies; // Removed
    // Removed Shader& shader;
    // Removed GLuint vao;
    // Removed GLuint vbo;
    int screenWidth;
    int screenHeight;
    float time;
    float zoomLevel;
    float rotationSpeed; // For Circular
    glm::vec3 lowColor;
    glm::vec3 midColor;
    glm::vec3 highColor;
    float vizSmoothingFactor; // For smoothing updates within styles
    glm::mat4 viewMatrix; // Pass Visualizer's view matrix (used by 3D)
    // NO projectionMatrix here - style provides it via getProjectionMatrix
};

class IVisualizationStyle {
public:
    virtual ~IVisualizationStyle() = default;

    // Initialize style-specific resources (shaders, buffers)
    virtual void init() = 0;

    // Main render method for a style
    virtual void render(RenderParameters& params) = 0;

    // Cleanup style-specific resources
    virtual void cleanup() = 0;

    // Optional: Method to update internal smoothed data if needed
    // virtual void updateSmoothedData(const std::vector<float>& newEnergies) {};

    // Method to get the projection matrix for this style
    virtual glm::mat4 getProjectionMatrix(int width, int height) const = 0;

    // Color update handling (optional, provide default implementation)
    virtual void updateColors(const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high) {
        // Default implementation does nothing, styles can override if needed
        (void)low; (void)mid; (void)high; // Mark as unused
    }

    // Settings update handling (optional, provide default implementation)
    virtual void updateSettings(float rotationSpeed, float zoomLevel) {
        // Default implementation does nothing, styles can override if needed
        (void)rotationSpeed; (void)zoomLevel; // Mark as unused
    }

    // Window resize handling
    virtual void resize(int width, int height) = 0;
}; 