#include "visualizer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Visualizer::Visualizer(AudioProcessor& audioProcessor)
    : m_audioProcessor(audioProcessor), m_style(VisualizationStyle::BAR_GRAPH),
      m_rotationSpeed(1.0f), m_zoomLevel(1.0f), m_width(0), m_height(0),
      m_VAO(0), m_VBO(0), m_shaderProgram(0),
      m_lowColor(0.1f, 0.4f, 0.8f), m_midColor(0.0f, 0.8f, 0.6f), m_highColor(1.0f, 0.2f, 0.4f) {}

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

    m_shader.use();

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)m_width / (float)m_height, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_shader.setMat4("projection", projection);
    m_shader.setMat4("view", view);

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
}

void Visualizer::updateSettings(VisualizationStyle style, float rotationSpeed, float zoomLevel) {
    m_style = style;
    m_rotationSpeed = rotationSpeed;
    m_zoomLevel = zoomLevel;
}

void Visualizer::renderBarGraph() {
    const std::vector<float>& bandEnergies = m_audioProcessor.getBandEnergies();
    
    // Set up viewport and projection with padding
    glViewport(0, 0, m_width, m_height);
    float padding = m_width * 0.05f; // 5% padding
    
    glm::mat4 projection = glm::ortho(
        -padding, 
        (float)m_width + padding, 
        -padding, 
        (float)m_height + padding
    );
    m_shader.setMat4("projection", projection);
    
    // Calculate bar width and spacing
    float totalWidth = m_width - 2 * padding;
    float barWidth = totalWidth / (bandEnergies.size() * 2.0f);
    float spacing = barWidth * 0.5f;
    
    // Draw bars with improved visuals
    float time = glfwGetTime();
    float baseHeight = m_height * 0.8f; // 80% of screen height
    
    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float x = padding + i * (barWidth + spacing);
        float normalizedEnergy = std::min(bandEnergies[i], 1.0f);
        
        // Add smooth animation
        float wave = sin(time * 1.5f + i * 0.1f) * 0.05f + 0.95f;
        float height = normalizedEnergy * baseHeight * wave * m_zoomLevel;
        
        // Create gradient color based on frequency and energy
        float hue = (float)i / bandEnergies.size();
        glm::vec3 color = interpolateColor(hue, normalizedEnergy);
        
        // Draw bar with rounded corners and glow effect
        float vertices[] = {
            x, padding, 0.0f, color.r * 0.7f, color.g * 0.7f, color.b * 0.7f,
            x + barWidth, padding, 0.0f, color.r * 0.7f, color.g * 0.7f, color.b * 0.7f,
            x + barWidth, height + padding, 0.0f, color.r, color.g, color.b,
            x, height + padding, 0.0f, color.r, color.g, color.b
        };
        
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        // Enable blending for glow effect
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        glDisable(GL_BLEND);
    }
}

void Visualizer::renderCircular() {
    const std::vector<float>& bandEnergies = m_audioProcessor.getBandEnergies();
    
    // Center the visualization
    glViewport(0, 0, m_width, m_height);
    float aspect = (float)m_width / m_height;
    
    glm::mat4 projection = glm::ortho(
        -aspect, aspect,
        -1.0f, 1.0f,
        -1.0f, 1.0f
    );
    m_shader.setMat4("projection", projection);
    
    // Calculate circle parameters
    float baseRadius = 0.3f * m_zoomLevel;
    float time = glfwGetTime() * m_rotationSpeed;
    
    // Enable blending for smooth transitions
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw circular visualization
    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float angle = (2.0f * M_PI * i) / bandEnergies.size() + time;
        float nextAngle = (2.0f * M_PI * (i + 1)) / bandEnergies.size() + time;
        
        float normalizedEnergy = std::min(bandEnergies[i], 1.0f);
        float wave = sin(time * 2.0f + i * 0.1f) * 0.1f + 0.9f;
        float currentRadius = baseRadius * (1.0f + normalizedEnergy * wave);
        
        // Create smooth color gradient
        float hue = (float)i / bandEnergies.size();
        glm::vec3 color = interpolateColor(hue, normalizedEnergy);
        
        // Draw segment with inner and outer radius
        float innerRadius = baseRadius * 0.8f;
        std::vector<float> vertices;
        
        // Inner vertices
        vertices.push_back(cos(angle) * innerRadius);
        vertices.push_back(sin(angle) * innerRadius);
        vertices.push_back(0.0f);
        vertices.push_back(color.r * 0.5f);
        vertices.push_back(color.g * 0.5f);
        vertices.push_back(color.b * 0.5f);
        
        vertices.push_back(cos(nextAngle) * innerRadius);
        vertices.push_back(sin(nextAngle) * innerRadius);
        vertices.push_back(0.0f);
        vertices.push_back(color.r * 0.5f);
        vertices.push_back(color.g * 0.5f);
        vertices.push_back(color.b * 0.5f);
        
        // Outer vertices
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
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    
    glDisable(GL_BLEND);
}

void Visualizer::renderWave() {
    const std::vector<float>& audioData = m_audioProcessor.getAudioData();
    
    // Set up viewport and projection with padding
    glViewport(0, 0, m_width, m_height);
    float padding = m_height * 0.2f; // 20% vertical padding
    
    glm::mat4 projection = glm::ortho(
        0.0f, (float)m_width,
        -m_height/2.0f + padding,
        m_height/2.0f - padding
    );
    m_shader.setMat4("projection", projection);
    
    // Enable anti-aliasing and blending
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Generate smooth wave
    std::vector<float> vertices;
    float time = glfwGetTime();
    
    int numPoints = m_width;
    float step = (float)audioData.size() / numPoints;
    
    for (int i = 0; i < numPoints; i++) {
        float x = (float)i;
        float t = i * step;
        int index = (int)t;
        float frac = t - index;
        
        // Smooth interpolation between samples
        float y = 0;
        if (index < (int)audioData.size() - 1) {
            y = audioData[index] * (1.0f - frac) + audioData[index + 1] * frac;
        } else if (index < (int)audioData.size()) {
            y = audioData[index];
        }
        
        // Add some subtle motion
        y *= m_zoomLevel * (1.0f + sin(time * 2.0f + x * 0.01f) * 0.1f);
        
        // Create smooth color gradient
        float hue = (float)i / numPoints;
        glm::vec3 color = interpolateColor(hue, std::abs(y));
        
        vertices.push_back(x);
        vertices.push_back(y * m_height * 0.4f);
        vertices.push_back(0.0f);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
    }
    
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Draw with thicker lines for better visibility
    glLineWidth(2.0f);
    glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 6);
    
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}

glm::vec3 Visualizer::interpolateColor(float hue, float energy) {
    // Enhanced color interpolation with energy-based effects
    float saturation = 0.7f + energy * 0.3f;  // More subtle saturation range
    float value = 0.6f + energy * 0.4f;       // Brighter base value
    
    // HSV to RGB conversion with improved color transitions
    float c = value * saturation;
    float x = c * (1.0f - std::abs(std::fmod(hue * 6.0f, 2.0f) - 1.0f));
    float m = value - c;
    
    glm::vec3 color;
    if (hue < 1.0f/6.0f) {
        color = glm::vec3(c, x, 0.0f);
    } else if (hue < 2.0f/6.0f) {
        color = glm::vec3(x, c, 0.0f);
    } else if (hue < 3.0f/6.0f) {
        color = glm::vec3(0.0f, c, x);
    } else if (hue < 4.0f/6.0f) {
        color = glm::vec3(0.0f, x, c);
    } else if (hue < 5.0f/6.0f) {
        color = glm::vec3(x, 0.0f, c);
    } else {
        color = glm::vec3(c, 0.0f, x);
    }
    
    return color + glm::vec3(m);
}