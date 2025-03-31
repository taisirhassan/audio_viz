#include "visualizer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Visualizer::Visualizer(AudioProcessor& audioProcessor)
    : m_audioProcessor(audioProcessor), m_style(VisualizationStyle::BAR_GRAPH),
      m_rotationSpeed(1.0f), m_zoomLevel(1.0f), m_width(0), m_height(0),
      m_VAO(0), m_VBO(0), m_shaderProgram(0),
      m_customLowColor(0.1f, 0.4f, 0.8f),
      m_customMidColor(0.0f, 0.8f, 0.6f),
      m_customHighColor(1.0f, 0.2f, 0.4f) {}

Visualizer::~Visualizer() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
}

bool Visualizer::initialize(int width, int height) {
    m_width = width;
    m_height = height;

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    // Create and compile shaders
    m_shader = Shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    m_shaderProgram = m_shader.ID;

    // Create VAO and VBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Visualizer::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable blending for smooth transitions
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use shader program
    m_shader.use();

    // Reset viewport to window size
    glViewport(0, 0, m_width, m_height);

    // Set up basic matrices with identity view matrix for 2D rendering
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);  // Identity view matrix for 2D
    
    // Use orthographic projection that matches window dimensions exactly
    float aspect = static_cast<float>(m_width) / m_height;
    glm::mat4 projection = glm::ortho(
        -aspect, aspect,  // Make sure we fill the width
        -1.0f, 1.0f,     // Keep height normalized
        -1.0f, 1.0f      // Near/far planes
    );

    // Set uniforms
    m_shader.setMat4("model", model);
    m_shader.setMat4("view", view);
    m_shader.setMat4("projection", projection);

    // Render based on style
    switch (m_style) {
        case VisualizationStyle::BAR_GRAPH:
            renderBarGraph();
            break;
        case VisualizationStyle::CIRCULAR:
            renderCircular();
            break;
        case VisualizationStyle::WAVE:
            renderWave();
            break;
    }

    // Disable blending
    glDisable(GL_BLEND);
}

void Visualizer::updateSettings(VisualizationStyle style, float rotationSpeed, float zoomLevel) {
    m_style = style;
    m_rotationSpeed = rotationSpeed;
    m_zoomLevel = zoomLevel;
}

void Visualizer::updateColors(const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high) {
    m_customLowColor = low;
    m_customMidColor = mid;
    m_customHighColor = high;
}

void Visualizer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, m_width, m_height); // Update viewport immediately
}

void Visualizer::renderBarGraph() {
    const std::vector<float>& bandEnergies = m_audioProcessor.getBandEnergies();
    if (bandEnergies.empty()) return;

    float aspect = static_cast<float>(m_width) / m_height;
    float barWidth = (aspect * 2.0f) / bandEnergies.size(); // Full width
    float spacing = barWidth * 0.05f; // Minimal spacing
    float totalWidth = barWidth - spacing;
    float startX = -aspect; // Start at left edge

    float time = static_cast<float>(glfwGetTime());

    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float x = startX + i * barWidth;
        float normalizedEnergy = std::min(bandEnergies[i], 1.0f);
        
        // Add smooth animation
        float wave = sin(time * 2.0f + i * 0.1f) * 0.1f + 0.9f;
        // Scale height to fill from -1 to +1, adjusted by zoom
        float height = normalizedEnergy * wave * m_zoomLevel * 2.0f - 1.0f; 
        
        // Create gradient color based on frequency and energy
        float hue = static_cast<float>(i) / bandEnergies.size();
        glm::vec3 color = interpolateColor(hue, normalizedEnergy);
        
        float bottomY = -1.0f; // Always start at the bottom
        float topY = std::min(1.0f, height); // Clamp top to screen boundary

        float vertices[] = {
            x,              bottomY, 0.0f,  color.r * 0.5f, color.g * 0.5f, color.b * 0.5f,
            x + totalWidth, bottomY, 0.0f,  color.r * 0.5f, color.g * 0.5f, color.b * 0.5f,
            x + totalWidth, topY,    0.0f,  color.r, color.g, color.b,
            x,              topY,    0.0f,  color.r, color.g, color.b
        };

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void Visualizer::renderCircular() {
    const std::vector<float>& bandEnergies = m_audioProcessor.getBandEnergies();
    if (bandEnergies.empty()) return;

    float aspect = static_cast<float>(m_width) / m_height;
    float time = glfwGetTime() * m_rotationSpeed;
    
    // Scale the base radius based on the smaller screen dimension, closer but not extreme
    float baseRadius = std::min(aspect, 1.0f) * 0.7f * m_zoomLevel; // Reduced from 0.9
    
    // Draw circular visualization
    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float angle = (2.0f * M_PI * i) / bandEnergies.size() - time;  // Keep reversed rotation
        float nextAngle = (2.0f * M_PI * (i + 1)) / bandEnergies.size() - time;
        
        // More sensitive energy scaling, allow reaching closer to edge, cap lower
        float normalizedEnergy = std::min(bandEnergies[i] * 1.1f, 1.2f) * m_zoomLevel; // Reduced factors
        float wave = sin(time * 1.5f + i * 0.1f) * 0.08f + 0.92f; // Slower, less intense wave
        // Scale radius more reasonably
        float currentRadius = baseRadius * (1.0f + normalizedEnergy * wave * 0.6f); // Reduced multiplier
        currentRadius = std::min(currentRadius, std::min(aspect, 1.0f) * 1.0f); // Cap at screen edge
        
        // Create smooth color gradient
        float hue = static_cast<float>(i) / bandEnergies.size();
        glm::vec3 color = interpolateColor(hue, normalizedEnergy / (1.2f * m_zoomLevel)); // Adjust normalization
        
        // Draw segment with inner and outer radius
        float innerRadius = baseRadius * 0.3f; // Keep inner radius reasonable
        std::vector<float> vertices;
        vertices.reserve(24);
        
        // Inner vertices - slightly brighter
        vertices.push_back(cos(angle) * innerRadius);
        vertices.push_back(sin(angle) * innerRadius);
        vertices.push_back(0.0f);
        vertices.push_back(color.r * 0.3f); 
        vertices.push_back(color.g * 0.3f);
        vertices.push_back(color.b * 0.3f);
        
        vertices.push_back(cos(nextAngle) * innerRadius);
        vertices.push_back(sin(nextAngle) * innerRadius);
        vertices.push_back(0.0f);
        vertices.push_back(color.r * 0.3f);
        vertices.push_back(color.g * 0.3f);
        vertices.push_back(color.b * 0.3f);
        
        // Outer vertices with full color
        vertices.push_back(cos(nextAngle) * currentRadius);
        vertices.push_back(sin(nextAngle) * currentRadius);
        vertices.push_back(0.0f);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        
        vertices.push_back(cos(angle) * currentRadius);
        vertices.push_back(sin(angle) * currentRadius);
        vertices.push_back(0.0f);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void Visualizer::renderWave() {
    const std::vector<float>& audioData = m_audioProcessor.getAudioData();
    if (audioData.empty()) return;

    float aspect = static_cast<float>(m_width) / m_height;
    float time = glfwGetTime();
    
    std::vector<float> vertices;
    vertices.reserve(m_width * 6);
    
    float step = static_cast<float>(audioData.size()) / m_width;
    // Scale amplitude to potentially fill screen height, adjusted by zoom
    float amplitudeScale = m_zoomLevel * 0.8f; // Reduced from 0.9
    
    for (int i = 0; i < m_width; i++) {
        // Map x coordinates to fill the entire width [-aspect, aspect]
        float x = (static_cast<float>(i) / m_width) * (aspect * 2.0f) - aspect;
        float t = i * step;
        int index = static_cast<int>(t);
        float frac = t - index;
        
        float y = 0.0f;
        if (index < static_cast<int>(audioData.size()) - 1) {
            y = audioData[index] * (1.0f - frac) + audioData[index + 1] * frac;
        } else if (index < static_cast<int>(audioData.size())) {
            y = audioData[index];
        }
        
        // Add subtle motion and scale amplitude, reduce the strong multiplier
        float wave = 1.0f + sin(time * 1.5f + x * 0.01f) * 0.08f; // Slower, less intense wave
        y *= amplitudeScale * wave * 5.0f; // Drastically reduced multiplier from 20.0
        y = std::max(-1.0f, std::min(1.0f, y)); // Keep clamping
        
        // Create smooth color gradient
        float progress = static_cast<float>(i) / m_width;
        float energy = std::abs(y); // Use clamped y for energy color
        glm::vec3 color = interpolateColor(progress, energy);
        
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
    }
    
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    glLineWidth(4.0f); // Slightly thicker line again
    glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 6);
}

glm::vec3 Visualizer::interpolateColor(float hue, float energy) {
    // Create a smooth transition between colors based on both hue and energy
    glm::vec3 color;
    
    // Use hue to determine the base color
    float hueNormalized = fmod(hue + energy * 0.2f, 1.0f); // Shift hue slightly based on energy
    
    if (hueNormalized < 0.33f) {
        float t = hueNormalized / 0.33f;
        color = glm::mix(m_customLowColor, m_customMidColor, t);
    } else if (hueNormalized < 0.66f) {
        float t = (hueNormalized - 0.33f) / 0.33f;
        color = glm::mix(m_customMidColor, m_customHighColor, t);
    } else {
        float t = (hueNormalized - 0.66f) / 0.33f;
        color = glm::mix(m_customHighColor, m_customLowColor, t);
    }
    
    // Add some brightness based on energy
    float brightness = 0.5f + energy * 0.5f;
    return color * brightness;
}