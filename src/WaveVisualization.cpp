#include <GL/glew.h> // Include GLEW first!
#include "WaveVisualization.h"
#include <GLFW/glfw3.h> // For glfwGetTime
#include <vector>
#include <algorithm> // For std::min/max

glm::vec3 WaveVisualization::interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high) {
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

void WaveVisualization::render(RenderParameters& params) {
    const auto& audioData = params.audioData;
    if (audioData.empty()) return;

    float aspect = static_cast<float>(params.screenWidth) / params.screenHeight;
    
    std::vector<float> vertices;
    // Estimate size needed: screenWidth pixels * 6 floats per vertex (pos+color)
    vertices.reserve(params.screenWidth * 6); 
    
    // Calculate step based on potentially varying screen width
    float step = audioData.size() > 0 ? static_cast<float>(audioData.size()) / params.screenWidth : 0;
    float amplitudeScale = params.zoomLevel * 0.8f;
    
    glBindVertexArray(params.vao);
    glBindBuffer(GL_ARRAY_BUFFER, params.vbo);

    for (int i = 0; i < params.screenWidth; i++) {
        float x = (static_cast<float>(i) / params.screenWidth) * (aspect * 2.0f) - aspect;
        float t = i * step;
        int index = static_cast<int>(t);
        float frac = t - index;
        
        float y = 0.0f;
        if (index < static_cast<int>(audioData.size()) - 1) {
            y = audioData[index] * (1.0f - frac) + audioData[index + 1] * frac;
        } else if (index < static_cast<int>(audioData.size())) {
            y = audioData[index];
        }
        
        float wave = 1.0f + sin(params.time * 1.5f + x * 0.01f) * 0.08f;
        y *= amplitudeScale * wave * 5.0f;
        y = std::max(-1.0f, std::min(1.0f, y));
        
        float progress = static_cast<float>(i) / params.screenWidth;
        float energy = std::abs(y);
        glm::vec3 color = interpolateColor(progress, energy, params.lowColor, params.midColor, params.highColor);
        
        vertices.push_back(x); vertices.push_back(y); vertices.push_back(0.0f);
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
    }
    
    // Only buffer and draw if vertices were generated
    if (!vertices.empty()) {
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        glLineWidth(4.0f);
        glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 6);
    }

    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);
} 