#pragma once

#include <vector>
#include <glm/glm.hpp> // For matrices and vectors
#include "shader.h"     // For passing the shader

// Forward declarations if needed
class AudioProcessor;

// Struct to pass render parameters efficiently
struct RenderParameters {
    const std::vector<float>& bandEnergies;
    const std::vector<float>& audioData; // For Wave
    const std::vector<float>& smoothedEnergies; // For smooth circular/bar
    Shader& shader;
    GLuint vao;
    GLuint vbo;
    int screenWidth;
    int screenHeight;
    float time;
    float zoomLevel;
    float rotationSpeed; // For Circular
    glm::vec3 lowColor;
    glm::vec3 midColor;
    glm::vec3 highColor;
    float vizSmoothingFactor; // For smoothing updates within styles
};

class IVisualizationStyle {
public:
    virtual ~IVisualizationStyle() = default;

    // Main render method for a style
    virtual void render(RenderParameters& params) = 0;

    // Optional: Method to update internal smoothed data if needed
    // virtual void updateSmoothedData(const std::vector<float>& newEnergies) {};
}; 