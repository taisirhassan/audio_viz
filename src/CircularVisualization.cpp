#include <GL/glew.h> // Include GLEW first!
#include "CircularVisualization.h"
#include "../include/shader.h" // Make sure Shader class is included
#include <GLFW/glfw3.h> // For glfwGetTime, maybe pass time in params?
#include <algorithm> // For std::min
#include <vector>
#include <iostream> // For error checking
// Note: <cmath> and M_PI included via header

// Use the same base quad vertices as the Bar Graph (positions 0-1)
extern const float quadVertices[]; // Defined in BarGraphVisualization.cpp

CircularVisualization::CircularVisualization() {
    // Constructor: Initialization moved to init()
    // Smoothing vector will be sized in init() or first render()
}

CircularVisualization::~CircularVisualization() {
    // Destructor: Cleanup is handled by cleanup()
    if (m_vao != 0) {
       // Optional warning if cleanup wasn't called
    }
}

void CircularVisualization::init() {
    try {
        // 1. Compile shaders
        m_shader = std::make_unique<Shader>("shaders/circular_vertex.glsl", "shaders/circular_fragment.glsl");

        // 2. Setup VAO and VBO for the base quad (same geometry as BarGraph)
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), quadVertices, GL_STATIC_DRAW); // 6 vertices, 2 floats each

        // Position attribute (vec2)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // 3. Setup Texture Buffer Object (TBO) for smoothed energies
        glGenBuffers(1, &m_tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
        glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // Initialize size later

        glGenTextures(1, &m_tboTexture);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_tbo); // Associate TBO with texture (float format)

        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        glBindTexture(GL_TEXTURE_BUFFER, 0);

        // Note: m_smoothedEnergies will be sized on the first render call based on actual data size

    } catch (const std::exception& e) {
        std::cerr << "Error initializing CircularVisualization: " << e.what() << std::endl;
        cleanup(); 
        throw; 
    }
}

void CircularVisualization::cleanup() {
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

void CircularVisualization::render(RenderParameters& params) {
    if (!m_shader || m_vao == 0 || params.bandEnergies.empty()) {
        return; // Not initialized or no data
    }

    const auto& bandEnergies = params.bandEnergies;
    size_t numSegments = bandEnergies.size();

    // --- Smoothing Logic --- 
    // Ensure smoothed energies vector is correctly sized
    if (m_smoothedEnergies.size() != numSegments) {
        m_smoothedEnergies.resize(numSegments, 0.0f);
    }
    // Calculate smoothed values for this frame
    for (size_t i = 0; i < numSegments; ++i) {
         float targetNormalizedEnergy = std::min(bandEnergies[i] * 1.1f, 1.2f) * params.zoomLevel; // Original logic
         m_smoothedEnergies[i] += (targetNormalizedEnergy - m_smoothedEnergies[i]) * params.vizSmoothingFactor;
         // Keep smoothed energy unclamped here, shader might handle visual clamping/scaling
    }
    // --- End Smoothing --- 

    m_shader->use();

    // 1. Update TBO with current SMOOTHED band energies
    glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
    glBufferData(GL_TEXTURE_BUFFER, m_smoothedEnergies.size() * sizeof(float), m_smoothedEnergies.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // 2. Bind TBO texture
    glActiveTexture(GL_TEXTURE0 + TBO_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
    m_shader->setInt("smoothedEnergiesSampler", TBO_TEXTURE_UNIT);

    // 3. Set uniforms
    m_shader->setMat4("view", params.viewMatrix);
    m_shader->setMat4("projection", params.projectionMatrix);
    m_shader->setFloat("numSegments", static_cast<float>(numSegments));
    m_shader->setFloat("zoomLevel", params.zoomLevel);
    m_shader->setFloat("rotationSpeed", params.rotationSpeed);
    m_shader->setFloat("time", params.time);
    m_shader->setVec3("lowColor", params.lowColor);
    m_shader->setVec3("midColor", params.midColor);
    m_shader->setVec3("highColor", params.highColor);
    // Pass factors needed by shader based on original C++ logic
    float aspect = (params.screenHeight > 0) ? static_cast<float>(params.screenWidth) / params.screenHeight : 1.0f;
    m_shader->setFloat("baseRadiusFactor", std::min(aspect, 1.0f) * 0.7f); // Base radius scale
    m_shader->setFloat("innerRadiusFactor", 0.3f); // Inner radius relative scale

    // 4. Bind VAO and draw instanced
    glBindVertexArray(m_vao);
    // Draw quads using the 6 vertices defined for triangles
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(numSegments)); 

    // 5. Unbind
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glUseProgram(0);
} 