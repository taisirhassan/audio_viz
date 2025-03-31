#include <GL/glew.h> // Include GLEW first!
#include "BarGraphVisualization.h"
#include <GLFW/glfw3.h> // For glfwGetTime
#include <algorithm> // For std::min

BarGraphVisualization::BarGraphVisualization() {
    // Constructor logic if needed
}

glm::vec3 BarGraphVisualization::interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high) {
    glm::vec3 color;
    float hueNormalized = fmod(hue + energy * 0.2f, 1.0f);
    if (hueNormalized < 0.33f) {
        color = glm::mix(low, mid, hueNormalized / 0.33f);
    } else if (hueNormalized < 0.66f) {
        color = glm::mix(mid, high, (hueNormalized - 0.33f) / 0.33f);
    } else {
        color = glm::mix(high, low, (hueNormalized - 0.66f) / 0.33f);
    }
    float brightness = 0.5f + energy * 0.5f;
    return color * brightness;
}

void BarGraphVisualization::render(RenderParameters& params) {
    const auto& bandEnergies = params.bandEnergies; // Using bandEnergies directly for now
    if (bandEnergies.empty()) return;

    float aspect = static_cast<float>(params.screenWidth) / params.screenHeight;
    float barWidth = (aspect * 2.0f) / bandEnergies.size(); // Full width
    float spacing = barWidth * 0.05f; // Minimal spacing
    float totalBarWidth = barWidth - spacing;
    float startX = -aspect; // Start at left edge

    glBindVertexArray(params.vao);
    glBindBuffer(GL_ARRAY_BUFFER, params.vbo);

    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float x = startX + i * barWidth;
        float normalizedEnergy = std::min(bandEnergies[i], 1.0f);
        
        float wave = sin(params.time * 2.0f + i * 0.1f) * 0.1f + 0.9f;
        float height = normalizedEnergy * wave * params.zoomLevel * 2.0f - 1.0f; // Adjusted scale
        
        float hue = static_cast<float>(i) / bandEnergies.size();
        glm::vec3 color = interpolateColor(hue, normalizedEnergy, params.lowColor, params.midColor, params.highColor);
        
        float bottomY = -1.0f; // Always start at the bottom
        float topY = std::min(1.0f, height); // Clamp top

        if (topY <= bottomY + 1e-6) { // Skip zero-height bars
            continue;
        }

        // Define vertices for a single bar
        float vertices[] = {
            x,               bottomY, 0.0f,  color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, // Bottom left, darker
            x + totalBarWidth, bottomY, 0.0f,  color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, // Bottom right, darker
            x + totalBarWidth, topY,    0.0f,  color.r, color.g, color.b,                   // Top right, full color
            x,               topY,    0.0f,  color.r, color.g, color.b                    // Top left, full color
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);
} 