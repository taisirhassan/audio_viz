// #define GLM_ENABLE_EXPERIMENTAL <-- Removed, now defined in CMake

#include <GL/glew.h> // Include GLEW first!
#include "WaveVisualization.h"
#include "../include/shader.h" // Make sure Shader class is included
#include <GLFW/glfw3.h> // For glfwGetTime
#include <algorithm> // For std::min
#include <vector>
#include <iostream> // For error checking
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>        // For glm::value_ptr
#include <glm/gtx/string_cast.hpp>     // For glm::to_string
#include "visualization_common.h" // Include common vertex data

WaveVisualization::WaveVisualization() {
    // Constructor: Initialize matrix to identity
    m_projectionOrtho = glm::mat4(1.0f);
}

WaveVisualization::~WaveVisualization() {
    // Destructor: Just ensure cleanup is called
    cleanup();
}

void WaveVisualization::init() {
    try {
        // 1. Compile shaders
        m_shader = std::make_unique<Shader>("shaders/wave_vertex.glsl", "shaders/wave_fragment.glsl");
        
        if (!m_shader->isValid()) {
            std::cerr << "Failed to compile Wave shader!" << std::endl;
            throw std::runtime_error("Wave shader compilation failed");
        }

        // 2. Setup VAO and VBO for wave lines
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        // Create line vertices for wave visualization (just 2 points for a single line segment)
        float lineVertices[] = {
            0.0f, 0.0f,  // Start point
            1.0f, 0.0f   // End point - will be positioned in the shader
        };

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

        // Position attribute (vec2)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Set line width for thicker wave lines
        glLineWidth(2.0f);

        // Enable line smoothing
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        // --- 3. Setup TBO for audio data ---
        const size_t MAX_SAMPLES = 4096; // Support up to 4096 samples
        std::vector<float> initialTboData(MAX_SAMPLES, 0.0f); // Vector of zeros

        glGenBuffers(1, &m_tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
        
        // Initialize buffer with zeros
        glBufferData(GL_TEXTURE_BUFFER, MAX_SAMPLES * sizeof(float), initialTboData.data(), GL_DYNAMIC_DRAW);
        
        glGenTextures(1, &m_tboTexture);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_tbo);
        
        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);

        std::cout << "WaveVisualization initialized successfully." << std::endl;

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
    std::cout << "WaveVisualization cleaned up." << std::endl;
}

// --- Projection Matrix Calculation Helper ---
void WaveVisualization::calculateOrthoProjection(int width, int height) {
    // For wave visualization, we'll use an aspect-ratio preserving projection
    if (height == 0) height = 1; // Prevent division by zero
    float aspectRatio = static_cast<float>(width) / height;
    
    // This creates an orthographic projection that maps to -1 to +1 range on both axes
    // For wave visualization, this is ideal as we can draw our wave across the screen
    m_projectionOrtho = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);
    
    std::cout << "Wave 2D Projection: " << glm::to_string(m_projectionOrtho) << std::endl;
}

// --- Implementation of IVisualizationStyle methods ---

void WaveVisualization::resize(int width, int height) {
    // Recalculate projection on resize
    calculateOrthoProjection(width, height);
}

glm::mat4 WaveVisualization::getProjectionMatrix(int width, int height) const {
    // Just return the stored orthographic matrix - ignore parameters as we'll use stored values
    (void)width; (void)height; // Avoid unused parameter warnings
    return m_projectionOrtho;
}

void WaveVisualization::render(RenderParameters& params) {
    if (!m_shader || m_vao == 0) {
        return; // Not initialized
    }

    // For wave visualization, we use the audioData directly (rather than bandEnergies)
    const auto& audioData = params.audioData;
    size_t numSamples = audioData.size();
    
    if (numSamples == 0) {
        return; // No data to render
    }

    // Enable line smoothing for this render pass
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set line width for this render pass
    glLineWidth(2.0f);

    // Clear any leftover GL errors
    while (glGetError() != GL_NO_ERROR);
    
    // Update TBO data with audio samples - ensure we don't exceed buffer size
    const size_t MAX_SAMPLES = 4096; // Max buffer size from initialization
    size_t dataSize = std::min(numSamples, MAX_SAMPLES) * sizeof(float);
    
    glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, dataSize, audioData.data());
    
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after updating wave TBO: " << err << std::endl;
    }

    // Bind TBO texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
    
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after binding wave texture: " << err << std::endl;
    }

    m_shader->use();
    
    // Try multiple uniform names for the audio sampler
    GLint audioSamplerLocation = glGetUniformLocation(m_shader->getID(), "audioSampler");
    if (audioSamplerLocation == -1) {
        audioSamplerLocation = glGetUniformLocation(m_shader->getID(), "audioDataSampler");
    }
    
    if (audioSamplerLocation != -1) {
        glUniform1i(audioSamplerLocation, 0); // Texture unit 0
    } else {
        std::cerr << "ERROR: Could not find uniform for audio data in wave shader!" << std::endl;
    }
    
    // Set the number of samples - limit to what we copied to the buffer
    int usableSamples = static_cast<int>(std::min(numSamples, MAX_SAMPLES));
    glUniform1i(glGetUniformLocation(m_shader->getID(), "numSamples"), usableSamples);
    
    // Set color uniforms (for colored waves)
    glUniform3fv(glGetUniformLocation(m_shader->getID(), "lowColor"), 1, glm::value_ptr(params.lowColor));
    glUniform3fv(glGetUniformLocation(m_shader->getID(), "midColor"), 1, glm::value_ptr(params.midColor));
    glUniform3fv(glGetUniformLocation(m_shader->getID(), "highColor"), 1, glm::value_ptr(params.highColor));
    
    // Set scaling and other parameters
    float waveScale = params.zoomLevel; // Adjust scale based on zoom
    glUniform1f(glGetUniformLocation(m_shader->getID(), "waveScale"), waveScale);
    
    // Set time for animations
    glUniform1f(glGetUniformLocation(m_shader->getID(), "time"), static_cast<float>(glfwGetTime()));
    
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after setting wave uniforms: " << err << std::endl;
    }
    
    // Set transformation matrices
    glUniformMatrix4fv(glGetUniformLocation(m_shader->getID(), "projection"), 1, GL_FALSE, glm::value_ptr(m_projectionOrtho));
    glUniformMatrix4fv(glGetUniformLocation(m_shader->getID(), "view"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniformMatrix4fv(glGetUniformLocation(m_shader->getID(), "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    // Bind VAO and draw
    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_LINES, 0, 2, static_cast<GLsizei>(usableSamples - 1));
    
    // Check for errors after drawing
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after drawing wave visualization: " << err << std::endl;
    }

    // Reset line width and disable line smoothing
    glLineWidth(1.0f);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);

    // Unbind everything
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glUseProgram(0);
} 