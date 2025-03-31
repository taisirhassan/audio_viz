#include <GL/glew.h> // Include GLEW first!
#include "WaveVisualization.h"
#include "../include/shader.h" // Make sure Shader class is included
#include <GLFW/glfw3.h> // For glfwGetTime
#include <vector>
#include <algorithm> // For std::min/max
#include <iostream>  // For error logging

// Define the base line segment vertices (x-coordinates 0 and 1)
const float lineVertices[] = {
    0.0f, // x for point 1
    1.0f  // x for point 2
};

WaveVisualization::WaveVisualization() {
    // Constructor: Initialization moved to init()
}

WaveVisualization::~WaveVisualization() {
    // Destructor: Cleanup handled by cleanup()
    if (m_vao != 0) { /* Optional warning */ }
}

void WaveVisualization::init() {
    try {
        // 1. Compile shaders
        m_shader = std::make_unique<Shader>("shaders/wave_vertex.glsl", "shaders/wave_fragment.glsl");

        // 2. Setup VAO and VBO for the base line segment
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

        // Position attribute (float)
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // 3. Setup Texture Buffer Object (TBO) for raw audio data
        glGenBuffers(1, &m_tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
        glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // Initialize size later

        glGenTextures(1, &m_tboTexture);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_tbo); // Associate TBO with texture (float format)

        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        glBindTexture(GL_TEXTURE_BUFFER, 0);

    } catch (const std::exception& e) {
        std::cerr << "Error initializing WaveVisualization: " << e.what() << std::endl;
        cleanup();
        throw;
    }
}

void WaveVisualization::cleanup() {
    if (m_shader) {
        m_shader.reset();
    }
    if (m_tboTexture != 0) {
        glDeleteTextures(1, &m_tboTexture);
        m_tboTexture = 0;
    }
    if (m_tbo != 0) {
        glDeleteBuffers(1, &m_tbo);
        m_tbo = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void WaveVisualization::render(RenderParameters& params) {
    if (!m_shader || m_vao == 0 || params.audioData.empty() || params.screenWidth <= 1) {
        return; // Not initialized, no data, or not enough width for segments
    }

    const auto& audioData = params.audioData;
    int numSegments = params.screenWidth - 1;
    if (numSegments <= 0) return;

    m_shader->use();

    // 1. Update TBO with current raw audio data
    glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
    glBufferData(GL_TEXTURE_BUFFER, audioData.size() * sizeof(float), audioData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // 2. Bind TBO texture
    glActiveTexture(GL_TEXTURE0 + TBO_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
    m_shader->setInt("audioDataSampler", TBO_TEXTURE_UNIT);

    // 3. Set uniforms
    m_shader->setMat4("view", params.viewMatrix);
    m_shader->setMat4("projection", params.projectionMatrix);
    m_shader->setFloat("screenWidth", static_cast<float>(params.screenWidth));
    m_shader->setFloat("aspectRatio", (params.screenHeight > 0) ? static_cast<float>(params.screenWidth) / params.screenHeight : 1.0f);
    m_shader->setFloat("zoomLevel", params.zoomLevel);
    m_shader->setFloat("time", params.time);
    m_shader->setVec3("lowColor", params.lowColor);
    m_shader->setVec3("midColor", params.midColor);
    m_shader->setVec3("highColor", params.highColor);
    m_shader->setInt("audioDataSize", static_cast<int>(audioData.size()));

    // 4. Bind VAO and draw instanced lines
    glBindVertexArray(m_vao);
    glLineWidth(4.0f); // Set line width
    // Draw numSegments instances, each instance uses the 2 vertices from the VBO (0.0f, 1.0f)
    glDrawArraysInstanced(GL_LINES, 0, 2, numSegments);

    // 5. Unbind
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glUseProgram(0);
} 