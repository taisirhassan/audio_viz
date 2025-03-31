#include <GL/glew.h> // Include GLEW first!
#include "BarGraphVisualization.h"
#include "../include/shader.h" // Make sure Shader class is included
#include <GLFW/glfw3.h> // For glfwGetTime
#include <algorithm> // For std::min
#include <vector>
#include <iostream> // For error checking
#include <glm/glm.hpp>

// Define vertices for a unit cube (centered at origin, -0.5 to 0.5)
// 36 vertices (6 faces * 2 triangles * 3 vertices), 3 floats per vertex (position only)
const float cubeVertices[] = {
    // Back face
    -0.5f, -0.5f, -0.5f, // Bottom-left
     0.5f,  0.5f, -0.5f, // top-right
     0.5f, -0.5f, -0.5f, // bottom-right         
     0.5f,  0.5f, -0.5f, // top-right
    -0.5f, -0.5f, -0.5f, // bottom-left
    -0.5f,  0.5f, -0.5f, // top-left
    // Front face
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    // Left face
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    // Right face
     0.5f,  0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,      
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
    // Bottom face
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,
    // Top face
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f,  0.5f,
};

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
        // 1. Compile 3D shaders
        m_shader = std::make_unique<Shader>("shaders/bar_graph_3d_vertex.glsl", "shaders/fragment.glsl");

        // 2. Setup VAO and VBO for the base CUBE
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        // Use cubeVertices now
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        // Position attribute (vec3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0);

        // 3. Setup TBO (remains the same, but pre-allocate size)
        const size_t MAX_BANDS = 2048; // Define a reasonable max size
        glGenBuffers(1, &m_tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
        // Allocate buffer storage initially
        glBufferData(GL_TEXTURE_BUFFER, MAX_BANDS * sizeof(float), nullptr, GL_DYNAMIC_DRAW); 
        glGenTextures(1, &m_tboTexture);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
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
        return; 
    }
    // std::cout << "  BarGraphVisualization::render START" << std::endl; // REMOVED DEBUG

    const auto& bandEnergies = params.bandEnergies;
    size_t numBars = bandEnergies.size();

    m_shader->use();

    // 1. Update TBO using glBufferSubData
    glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
    // Ensure we don't write past allocated size (though unlikely with audio data)
    size_t dataSize = std::min(bandEnergies.size(), (size_t)2048); // Use the MAX_BANDS limit
    glBufferSubData(GL_TEXTURE_BUFFER, 0, dataSize * sizeof(float), bandEnergies.data());
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // 2. Bind TBO texture (remains the same)
    glActiveTexture(GL_TEXTURE0 + TBO_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
    m_shader->setInt("bandEnergiesSampler", TBO_TEXTURE_UNIT);

    // 3. Set uniforms
    glm::mat4 model = glm::mat4(1.0f); // Simple identity model matrix for now
    // Optionally rotate or position the whole bar graph setup here
    // model = glm::rotate(model, glm::radians(some_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    m_shader->setMat4("model", model); // Pass model matrix
    m_shader->setMat4("view", params.viewMatrix);
    m_shader->setMat4("projection", params.projectionMatrix);
    m_shader->setFloat("aspectRatio", (params.screenHeight > 0) ? static_cast<float>(params.screenWidth) / params.screenHeight : 1.0f);
    m_shader->setFloat("numBars", static_cast<float>(numBars));
    m_shader->setFloat("zoomLevel", params.zoomLevel);
    m_shader->setFloat("time", params.time);
    m_shader->setVec3("lowColor", params.lowColor);
    m_shader->setVec3("midColor", params.midColor);
    m_shader->setVec3("highColor", params.highColor);

    // 4. Bind VAO and draw instanced cubes
    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, static_cast<GLsizei>(numBars)); // 36 vertices per cube

    // 5. Unbind (remains the same)
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glUseProgram(0);

    // std::cout << "  BarGraphVisualization::render END" << std::endl; // REMOVED DEBUG
} 