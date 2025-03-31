#include <GL/glew.h> // Include GLEW first!
#include "BarGraphVisualization.h"
#include "../include/shader.h" // Make sure Shader class is included
#include <GLFW/glfw3.h> // For glfwGetTime
#include <algorithm> // For std::min
#include <vector>
#include <iostream> // For error checking

// Define the base quad vertices with external linkage
extern const float quadVertices[] = {
    // Triangle 1
    0.0f, 0.0f, // Bottom-left
    1.0f, 0.0f, // Bottom-right
    1.0f, 1.0f, // Top-right
    // Triangle 2
    1.0f, 1.0f, // Top-right
    0.0f, 1.0f, // Top-left
    0.0f, 0.0f  // Bottom-left
};

BarGraphVisualization::BarGraphVisualization() {
    // Constructor: Initialization moved to init()
}

BarGraphVisualization::~BarGraphVisualization() {
    // Destructor: Cleanup moved to cleanup()
    // Ensure cleanup is called if the object is destroyed before explicit cleanup
    // Though typically cleanup() should be managed by the Visualizer class
    if (m_vao != 0) { 
       // std::cerr << "Warning: BarGraphVisualization destroyed without calling cleanup() first." << std::endl;
       // cleanup(); // Optionally call cleanup here, but prefer explicit management
    }
}

void BarGraphVisualization::init() {
    try {
        // 1. Compile shaders
        m_shader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");

        // 2. Setup VAO and VBO for the base quad
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        // Position attribute (vec2)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0);

        // 3. Setup Texture Buffer Object (TBO) for band energies
        glGenBuffers(1, &m_tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
        // We'll buffer the actual data in the render loop
        glBufferData(GL_TEXTURE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // Initialize size later

        glGenTextures(1, &m_tboTexture);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        // Associate the TBO with the texture, using a float format
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_tbo);

        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        glBindTexture(GL_TEXTURE_BUFFER, 0);

    } catch (const std::exception& e) {
        std::cerr << "Error initializing BarGraphVisualization: " << e.what() << std::endl;
        // Handle initialization error appropriately (e.g., set a flag, throw)
        cleanup(); // Clean up any partially created resources
        throw; // Re-throw the exception or handle as needed
    }
}

void BarGraphVisualization::cleanup() {
    if (m_shader) {
        // Shader destructor handles glDeleteProgram
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

void BarGraphVisualization::render(RenderParameters& params) {
    if (!m_shader || m_vao == 0 || params.bandEnergies.empty()) { 
        // Not initialized or no data
        return; 
    }

    const auto& bandEnergies = params.bandEnergies;
    size_t numBars = bandEnergies.size();

    m_shader->use();

    // 1. Update TBO with current band energies
    glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
    // Orphan the old buffer and allocate new storage if size changed, or just update data
    glBufferData(GL_TEXTURE_BUFFER, bandEnergies.size() * sizeof(float), bandEnergies.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // 2. Bind TBO texture to the assigned texture unit
    glActiveTexture(GL_TEXTURE0 + TBO_TEXTURE_UNIT); 
    glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
    m_shader->setInt("bandEnergiesSampler", TBO_TEXTURE_UNIT); // Tell shader which unit TBO is on

    // 3. Set uniforms
    m_shader->setMat4("view", params.viewMatrix);
    m_shader->setMat4("projection", params.projectionMatrix);
    m_shader->setFloat("aspectRatio", static_cast<float>(params.screenWidth) / params.screenHeight);
    m_shader->setFloat("numBars", static_cast<float>(numBars));
    m_shader->setFloat("zoomLevel", params.zoomLevel);
    m_shader->setFloat("time", params.time);
    m_shader->setVec3("lowColor", params.lowColor);
    m_shader->setVec3("midColor", params.midColor);
    m_shader->setVec3("highColor", params.highColor);

    // 4. Bind VAO and draw instanced
    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(numBars)); // 6 vertices per quad, numBars instances

    // 5. Unbind
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0); 
    glUseProgram(0);
} 