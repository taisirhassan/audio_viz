// #define GLM_ENABLE_EXPERIMENTAL  // Required for glm::to_string

#include <GL/glew.h> // Include GLEW first!
#include "CircularVisualization.h"
#include "../include/shader.h" // Make sure Shader class is included
#include <GLFW/glfw3.h> // For glfwGetTime, maybe pass time in params?
#include <algorithm> // For std::min
#include <vector>
#include <iostream> // For error checking
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>        // For glm::value_ptr
#include <glm/gtx/string_cast.hpp>     // For glm::to_string
#include "visualization_common.h" // Include common vertex data
// Note: <cmath> and M_PI included via header

// Use the same base quad vertices as the Bar Graph (positions 0-1)
extern const float quadVertices[]; // Defined in BarGraphVisualization.cpp

CircularVisualization::CircularVisualization() {
    // Constructor: Initialize matrix to identity
    m_projectionOrtho = glm::mat4(1.0f);
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

        GLenum err;
        while((err = glGetError()) != GL_NO_ERROR); // Clear previous errors

        // 2. Setup VAO and VBO for radial bars (using standard quad)
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        // Use the common quadVertices data
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * QUAD_VERTICES_SIZE, quadVertices, GL_STATIC_DRAW);
        
        // Position attribute (vec2)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        err = glGetError();
        if (err != GL_NO_ERROR) { std::cerr << "!!! Error setting up Circular VAO/VBO: " << err << std::endl; }

        // 3. Setup TBO (Texture Buffer Object) - keep this as before
        const size_t MAX_SEGMENTS = 2048; 
        std::vector<float> initialTboData(MAX_SEGMENTS, 0.0f);

        glGenBuffers(1, &m_tbo);
        err = glGetError();
        if (err != GL_NO_ERROR) { std::cerr << "!!! Error after glGenBuffers (TBO): " << err << std::endl; }
        
        glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
        err = glGetError();
        if (err != GL_NO_ERROR) { std::cerr << "!!! Error after glBindBuffer (TBO): " << err << std::endl; }
        
        glBufferData(GL_TEXTURE_BUFFER, MAX_SEGMENTS * sizeof(float), initialTboData.data(), GL_DYNAMIC_DRAW);
        err = glGetError();
        if (err != GL_NO_ERROR) { std::cerr << "!!! Error after glBufferData (TBO): " << err << std::endl; }
        
        glGenTextures(1, &m_tboTexture);
        err = glGetError();
         if (err != GL_NO_ERROR) { std::cerr << "!!! Error after glGenTextures (TBO Texture): " << err << std::endl; }
        
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        err = glGetError();
        if (err != GL_NO_ERROR) { std::cerr << "!!! Error after glBindTexture (TBO Texture): " << err << std::endl; }
        
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_tbo);
        err = glGetError(); // Check after association
        if (err != GL_NO_ERROR) {
            std::cerr << "!!! OpenGL Error after Circular glTexBuffer: " << err << std::endl;
        }

        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        err = glGetError();
        if (err != GL_NO_ERROR) { std::cerr << "!!! Error after unbinding TBO resources: " << err << std::endl; }

        std::cout << "CircularVisualization initialized successfully (using Quads)." << std::endl;

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

// --- Projection Matrix Calculation Helper ---
void CircularVisualization::calculateOrthoProjection(int width, int height) {
    // For circular visualization, we'll use an aspect-ratio preserving projection
    if (height == 0) height = 1; // Prevent division by zero
    float aspectRatio = static_cast<float>(width) / height;
    
    // This creates an orthographic projection that maps to -1 to +1 range on both axes
    // For circular visualization, this is ideal as we can draw our circle in the center
    m_projectionOrtho = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);
    
    std::cout << "Circular 2D Projection: " << glm::to_string(m_projectionOrtho) << std::endl;
}

// --- Implementation of IVisualizationStyle methods ---

void CircularVisualization::resize(int width, int height) {
    // Recalculate projection on resize
    calculateOrthoProjection(width, height);
}

glm::mat4 CircularVisualization::getProjectionMatrix(int width, int height) const {
    // Just return the stored orthographic matrix - ignore parameters as we'll use stored values
    // The parameters are provided to match the interface requirements
    (void)width; (void)height; // Avoid unused parameter warnings
    return m_projectionOrtho;
}

void CircularVisualization::render(RenderParameters& params) {
    if (!m_shader || m_vao == 0) {
        std::cerr << "CircularVisualization not initialized properly: shader=" 
                  << (m_shader ? m_shader->getID() : 0) << ", vao=" << m_vao << std::endl;
        return; // Not initialized
    }

    if (params.bandEnergies.empty()) {
        // Don't print error every frame, just return
        return; // No data
    }

    const auto& energies = params.bandEnergies;
    size_t numSegments = energies.size();

    // Clear any leftover GL errors BEFORE our operations
    while(glGetError() != GL_NO_ERROR);
            
    // Update TBO data
    glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
    size_t dataSize = numSegments * sizeof(float);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, dataSize, energies.data()); 
    glBindBuffer(GL_TEXTURE_BUFFER, 0); // Unbind after use
    
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after updating circular TBO: " << err << std::endl;
        // return; // Optional: stop rendering if TBO update failed
    }

    // Bind TBO texture to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
    
    // *** Force re-association of buffer with texture ***
    glBindBuffer(GL_TEXTURE_BUFFER, m_tbo); // Bind the buffer first
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_tbo); // Re-associate
    glBindBuffer(GL_TEXTURE_BUFFER, 0); // Unbind buffer
    // Now bind the texture again (might be redundant, but safe)
    glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture); 
    // *************************************************

    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after binding/re-associating circular texture: " << err << std::endl;
    }

    // Use shader
    m_shader->use();
    
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after shader->use(): " << err << std::endl;
       // return;
    }
    
    // Verify shader program is valid before getting locations
    if (!m_shader->isValid()) {
        std::cerr << "CircularVisualization: Shader program is invalid! ID: " << m_shader->getID() << std::endl;
        glBindTexture(GL_TEXTURE_BUFFER, 0); // Cleanup bind state
        return;
    }
    
    // --- Set Uniforms --- 
    
    // Energy Sampler (Try both names)
    GLint energyLoc = glGetUniformLocation(m_shader->getID(), "energyLevels");
    if (energyLoc == -1) {
        energyLoc = glGetUniformLocation(m_shader->getID(), "bandEnergiesSampler");
    }
    if (energyLoc != -1) {
        glUniform1i(energyLoc, 0); // Texture unit 0
    } else {
        std::cerr << "Warning: Could not find energy sampler uniform (energyLevels or bandEnergiesSampler) in circular shader!" << std::endl;
    }
    if (glGetError() != GL_NO_ERROR) std::cerr << "GL Error after setting energy sampler uniform" << std::endl;

    // Helper lambda to set uniform and check location/error
    auto setUniformFloat = [&](const char* name, float value) {
        GLint loc = glGetUniformLocation(m_shader->getID(), name);
        if (loc != -1) {
            glUniform1f(loc, value);
            if (glGetError() != GL_NO_ERROR) std::cerr << "GL Error setting float uniform: " << name << std::endl;
        } else {
            std::cerr << "Warning: Uniform location not found for: " << name << std::endl;
        }
    };
     auto setUniformInt = [&](const char* name, int value) {
        GLint loc = glGetUniformLocation(m_shader->getID(), name);
        if (loc != -1) {
            glUniform1i(loc, value);
            if (glGetError() != GL_NO_ERROR) std::cerr << "GL Error setting int uniform: " << name << std::endl;
        } else {
             std::cerr << "Warning: Uniform location not found for: " << name << std::endl;
        }
    };
     auto setUniformVec3 = [&](const char* name, const glm::vec3& value) {
        GLint loc = glGetUniformLocation(m_shader->getID(), name);
        if (loc != -1) {
            glUniform3fv(loc, 1, glm::value_ptr(value));
             if (glGetError() != GL_NO_ERROR) std::cerr << "GL Error setting vec3 uniform: " << name << std::endl;
        } else {
             std::cerr << "Warning: Uniform location not found for: " << name << std::endl;
        }
    };
    auto setUniformMat4 = [&](const char* name, const glm::mat4& value) {
        GLint loc = glGetUniformLocation(m_shader->getID(), name);
        if (loc != -1) {
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
            if (glGetError() != GL_NO_ERROR) std::cerr << "GL Error setting mat4 uniform: " << name << std::endl;
        } else {
             std::cerr << "Warning: Uniform location not found for: " << name << std::endl;
        }
    };

    // Other uniforms using the helper
    setUniformFloat("radius", params.zoomLevel * 0.8f);
    setUniformFloat("baseRadiusFactor", 0.3f); 
    setUniformFloat("innerRadiusFactor", 0.1f); 
    setUniformInt("numBars", static_cast<int>(numSegments));
    setUniformVec3("lowColor", params.lowColor);
    setUniformVec3("midColor", params.midColor);
    setUniformVec3("highColor", params.highColor);
    setUniformFloat("time", params.time);
    setUniformFloat("rotationSpeed", params.rotationSpeed);
    setUniformMat4("projection", m_projectionOrtho);
    setUniformMat4("view", glm::mat4(1.0f));
    setUniformMat4("model", glm::mat4(1.0f));

    // Check for errors after setting uniforms - THIS IS REDUNDANT NOW due to checks in helpers
    // err = glGetError();
    // if (err != GL_NO_ERROR) {
    //     std::cerr << "GL Error after setting circular uniforms: " << err << std::endl;
    // }

    // --- Draw Call --- 
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Disable line smooth, enable depth test if needed later for overlap
    // glEnable(GL_DEPTH_TEST); // Maybe needed if bars overlap?
    glDisable(GL_LINE_SMOOTH);
    // glLineWidth(2.0f); // Not needed for filled triangles
    
    // *** Explicitly bind texture to unit 0 ***
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
    // ******************************************
    
    // Bind the VAO for drawing
    glBindVertexArray(m_vao);
    
    err = glGetError(); 
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error before drawing circular Quads: " << err << std::endl;
    }
    
    // Draw the instanced quads (as triangles)
    if (numSegments > 0) {
        GLsizei instanceCount = static_cast<GLsizei>(numSegments);
        // Use GL_TRIANGLES and 6 vertices for the quad
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceCount); 
    }
    
    // Check for errors after draw call
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error after drawing circular Quads: " << err << std::endl;
    }
    
    // --- Cleanup --- 
    // glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    glBindVertexArray(0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0); 
} 