#include <GL/glew.h> // Include GLEW first!
#include "CircularVisualization.h"
#include <GLFW/glfw3.h> // For glfwGetTime, maybe pass time in params?
// Note: <cmath> and M_PI included via header

CircularVisualization::CircularVisualization() {
    // We don't know the number of bands yet. Resize will happen on first render.
}

glm::vec3 CircularVisualization::interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high) {
     // Create a smooth transition between colors based on both hue and energy
    glm::vec3 color;
    
    // Use hue to determine the base color
    float hueNormalized = fmod(hue + energy * 0.2f, 1.0f); // Shift hue slightly based on energy
    
    if (hueNormalized < 0.33f) {
        float t = hueNormalized / 0.33f;
        color = glm::mix(low, mid, t);
    } else if (hueNormalized < 0.66f) {
        float t = (hueNormalized - 0.33f) / 0.33f;
        color = glm::mix(mid, high, t);
    } else {
        float t = (hueNormalized - 0.66f) / 0.33f;
        color = glm::mix(high, low, t);
    }
    
    // Add some brightness based on energy
    float brightness = 0.5f + energy * 0.5f;
    return color * brightness;
}

void CircularVisualization::render(RenderParameters& params) {
    const auto& bandEnergies = params.bandEnergies;
    if (bandEnergies.empty()) return;

    // Ensure smoothed energies vector is correctly sized
    if (m_smoothedEnergies.size() != bandEnergies.size()) {
        m_smoothedEnergies.resize(bandEnergies.size(), 0.0f);
    }

    float aspect = static_cast<float>(params.screenWidth) / params.screenHeight;
    // Use time from parameters
    float rotatedTime = params.time * params.rotationSpeed;
    
    // Scale the base radius based on the smaller screen dimension
    float baseRadius = std::min(aspect, 1.0f) * 0.7f * params.zoomLevel;
    
    glBindVertexArray(params.vao);
    glBindBuffer(GL_ARRAY_BUFFER, params.vbo);

    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float angle = (2.0f * M_PI * i) / bandEnergies.size() - rotatedTime;
        float nextAngle = (2.0f * M_PI * (i + 1)) / bandEnergies.size() - rotatedTime;
        
        float targetNormalizedEnergy = std::min(bandEnergies[i] * 1.1f, 1.2f) * params.zoomLevel;

        // Smoothing Logic (using member vector)
        m_smoothedEnergies[i] += (targetNormalizedEnergy - m_smoothedEnergies[i]) * params.vizSmoothingFactor;
        float renderEnergy = m_smoothedEnergies[i];
        
        if (renderEnergy < 0.001f) { 
            continue;
        }
        
        float wave = sin(params.time * 1.5f + i * 0.1f) * 0.08f + 0.92f;
        float currentRadius = baseRadius * (1.0f + renderEnergy * wave * 0.6f);
        currentRadius = std::min(currentRadius, std::min(aspect, 1.0f) * 1.0f);
        
        float hue = static_cast<float>(i) / bandEnergies.size();
        glm::vec3 color = interpolateColor(hue, renderEnergy / (1.2f * params.zoomLevel), params.lowColor, params.midColor, params.highColor);
        
        float innerRadius = baseRadius * 0.3f;
        
        // Use a local vector for vertices per segment
        std::vector<float> vertices;
        vertices.reserve(24);
        
        // Inner vertices
        vertices.push_back(cos(angle) * innerRadius); vertices.push_back(sin(angle) * innerRadius); vertices.push_back(0.0f);
        vertices.push_back(color.r * 0.3f); vertices.push_back(color.g * 0.3f); vertices.push_back(color.b * 0.3f);
        vertices.push_back(cos(nextAngle) * innerRadius); vertices.push_back(sin(nextAngle) * innerRadius); vertices.push_back(0.0f);
        vertices.push_back(color.r * 0.3f); vertices.push_back(color.g * 0.3f); vertices.push_back(color.b * 0.3f);
        // Outer vertices
        vertices.push_back(cos(nextAngle) * currentRadius); vertices.push_back(sin(nextAngle) * currentRadius); vertices.push_back(0.0f);
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        vertices.push_back(cos(angle) * currentRadius); vertices.push_back(sin(angle) * currentRadius); vertices.push_back(0.0f);
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        
        // Buffer data and draw for this segment
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    
    // Unbind VBO/VAO ? Usually done outside the loop/style render
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);
} 