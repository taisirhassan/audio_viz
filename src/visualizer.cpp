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
    
    // Enable blending for smooth transitions
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use shader program
    m_shader.use();

    // Set up basic matrices
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 projection = glm::ortho(
        -1.0f * m_width / m_height, 1.0f * m_width / m_height,
        -1.0f, 1.0f,
        -1.0f, 100.0f
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

void Visualizer::renderBarGraph() {
    const std::vector<float>& bandEnergies = m_audioProcessor.getBandEnergies();
    if (bandEnergies.empty()) return;

    float barWidth = 1.8f / bandEnergies.size();
    float spacing = barWidth * 0.2f;
    float totalWidth = barWidth + spacing;
    float startX = -0.9f;

    float time = static_cast<float>(glfwGetTime());

    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float x = startX + i * totalWidth;
        float normalizedEnergy = std::min(bandEnergies[i], 1.0f);
        
        // Add smooth animation
        float wave = sin(time * 2.0f + i * 0.1f) * 0.1f + 0.9f;
        float height = normalizedEnergy * wave * m_zoomLevel;
        
        // Create gradient color based on frequency and energy
        float hue = static_cast<float>(i) / bandEnergies.size();
        glm::vec3 color = interpolateColor(hue, normalizedEnergy);
        
        float vertices[] = {
            x,            -0.9f, 0.0f,  color.r * 0.5f, color.g * 0.5f, color.b * 0.5f,  // bottom left
            x + barWidth, -0.9f, 0.0f,  color.r * 0.5f, color.g * 0.5f, color.b * 0.5f,  // bottom right
            x + barWidth, -0.9f + height * 1.8f, 0.0f,  color.r, color.g, color.b,        // top right
            x,            -0.9f + height * 1.8f, 0.0f,  color.r, color.g, color.b         // top left
        };

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
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
    // Create a smooth transition between colors based on both hue and energy
    glm::vec3 color;
    
    // Use hue to determine the base color
    float hueNormalized = fmod(hue + energy * 0.2f, 1.0f); // Shift hue slightly based on energy
    
    if (hueNormalized < 0.33f) {
        float t = hueNormalized / 0.33f;
        color = glm::mix(m_lowColor, m_midColor, t);
    } else if (hueNormalized < 0.66f) {
        float t = (hueNormalized - 0.33f) / 0.33f;
        color = glm::mix(m_midColor, m_highColor, t);
    } else {
        float t = (hueNormalized - 0.66f) / 0.33f;
        color = glm::mix(m_highColor, m_lowColor, t);
    }
    
    // Add some brightness based on energy
    float brightness = 0.5f + energy * 0.5f;
    return color * brightness;
}