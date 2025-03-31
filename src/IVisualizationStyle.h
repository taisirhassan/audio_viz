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
    glm::mat4 viewMatrix; // Added
    glm::mat4 projectionMatrix; // Added
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
}; 