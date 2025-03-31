// #define GLM_ENABLE_EXPERIMENTAL  <-- Removed, now defined in CMake

#include <GL/glew.h> // Include GLEW first!
#include "BarGraphVisualization.h"
#include "../include/shader.h" // Make sure Shader class is included
#include <GLFW/glfw3.h> // For glfwGetTime
#include <algorithm> // For std::min
#include <vector>
#include <iostream> // For error checking
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // For ortho matrix and transforms
#include <glm/gtx/string_cast.hpp>     // For glm::to_string
#include <glm/gtc/type_ptr.hpp>        // For matrix transformations
#include "visualization_common.h" // Include common vertex data

BarGraphVisualization::BarGraphVisualization() {
    // Constructor: Initialize matrices to identity
    m_projectionOrtho = glm::mat4(1.0f);
    m_projectionPerspective = glm::mat4(1.0f);
    m_is3D = false; // Default to 2D mode instead of 3D
}

// Add definition for destructor
BarGraphVisualization::~BarGraphVisualization() {
    cleanup(); // Ensure cleanup is called if object is destroyed
}

// Add definition for set3DMode
void BarGraphVisualization::set3DMode(bool is3D) {
    m_is3D = is3D;
    std::cout << "BarGraphVisualization: 3D mode set to " << (m_is3D ? "true" : "false") << std::endl;
}

void BarGraphVisualization::init() {
    try {
        // --- 1. Compile Shaders ---
        std::cout << "Compiling 2D shader..." << std::endl;
        m_shader2D = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");
        if (!m_shader2D->isValid()) {
            std::cerr << "Failed to compile 2D shader!" << std::endl;
            throw std::runtime_error("2D shader compilation failed");
        }

        std::cout << "Compiling 3D shader..." << std::endl;
        m_shader3D = std::make_unique<Shader>("shaders/bar_graph_3d_vertex.glsl", "shaders/fragment.glsl");
        if (!m_shader3D->isValid()) {
            std::cerr << "Failed to compile 3D shader!" << std::endl;
            throw std::runtime_error("3D shader compilation failed");
        }

        // --- 2. Setup 2D VAO/VBO ---
        glGenVertexArrays(1, &m_vao2D);
        glGenBuffers(1, &m_vbo2D);
        glBindVertexArray(m_vao2D);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo2D);
        glBufferData(GL_ARRAY_BUFFER, QUAD_VERTICES_SIZE * sizeof(float), quadVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // --- 3. Setup 3D VAO/VBO ---
        glGenVertexArrays(1, &m_vao3D);
        glGenBuffers(1, &m_vbo3D);
        
        glBindVertexArray(m_vao3D);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo3D);
        glBufferData(GL_ARRAY_BUFFER, CUBE_VERTICES_SIZE * sizeof(float), cubeVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // --- 4. Setup Shared TBO ---
        const size_t MAX_BANDS = 2048;
        std::vector<float> initialTboData(MAX_BANDS, 0.0f); // Create vector of zeros

        // Create and initialize the TBO
        glGenBuffers(1, &m_tbo);
        glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
        
        // Check for any errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after generating buffer: " << err << std::endl;
        }
        
        // Initialize buffer with zeros
        glBufferData(GL_TEXTURE_BUFFER, MAX_BANDS * sizeof(float), initialTboData.data(), GL_DYNAMIC_DRAW); 
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after buffer data: " << err << std::endl;
        }
        
        // Create texture for TBO
        glGenTextures(1, &m_tboTexture);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error after generating texture: " << err << std::endl;
        }
        
        // Associate buffer with texture
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_tbo); 
        
        // *** ADD ERROR CHECK HERE ***
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "!!! OpenGL Error after BarGraph glTexBuffer: " << err << std::endl;
        }
        // ***************************
        
        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);

        // Verify 3D shader uniforms
        m_shader3D->use();
        GLint samplerLoc = glGetUniformLocation(m_shader3D->getID(), "bandEnergiesSampler");
        if (samplerLoc == -1) {
            std::cerr << "WARNING: 'bandEnergiesSampler' uniform not found in shader!" << std::endl;
    } else {
            std::cout << "bandEnergiesSampler uniform location: " << samplerLoc << std::endl;
        }
        glUseProgram(0);

        std::cout << "BarGraphVisualization initialized successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error initializing BarGraphVisualization: " << e.what() << std::endl;
        cleanup();
        throw;
    }
}

// Add definition for cleanup
void BarGraphVisualization::cleanup() {
    glDeleteVertexArrays(1, &m_vao2D);
    glDeleteBuffers(1, &m_vbo2D);
    glDeleteVertexArrays(1, &m_vao3D);
    glDeleteBuffers(1, &m_vbo3D);
    glDeleteBuffers(1, &m_tbo);
    glDeleteTextures(1, &m_tboTexture);
    // Shaders are cleaned up by unique_ptr
    m_vao2D = m_vbo2D = m_vao3D = m_vbo3D = m_tbo = m_tboTexture = 0;
    std::cout << "BarGraphVisualization cleaned up." << std::endl;
}

// --- Projection Matrix Calculation Helpers ---
void BarGraphVisualization::calculateOrthoProjection(int width, int height) {
    // For 2D, we want an orthographic projection that maps:
    // x from -aspectRatio to +aspectRatio
    // y from -1 to +1
    
    if (height == 0) height = 1; // Prevent division by zero
    float aspectRatio = static_cast<float>(width) / height;
    
    // This maps NDC -1 to +1 on both axes, adjusted for aspect ratio
    m_projectionOrtho = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);
    
    std::cout << "Bar Graph 2D Projection: " << glm::to_string(m_projectionOrtho) << std::endl;
}

void BarGraphVisualization::calculatePerspectiveProjection(int width, int height) {
    if (height == 0) height = 1; // Prevent division by zero
    float aspectRatio = static_cast<float>(width) / height;
    m_projectionPerspective = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
}

// --- Implementation of IVisualizationStyle methods ---

void BarGraphVisualization::resize(int width, int height) {
    calculateOrthoProjection(width, height);
    calculatePerspectiveProjection(width, height);
}

glm::mat4 BarGraphVisualization::getProjectionMatrix(int width, int height) const {
    // Removed unused parameters warning by using them (or casting to void)
    (void)width; (void)height;
    return m_is3D ? m_projectionPerspective : m_projectionOrtho;
}

void BarGraphVisualization::render(RenderParameters& params) {
    size_t numBars = params.bandEnergies.size();
    if (numBars == 0) return;

    GLenum err; // <<< DECLARE err HERE

    // Print key rendering parameters
    std::cout << "BarGraphVisualization::render - "
              << "3D Mode: " << (m_is3D ? "Yes" : "No")
              << ", numBars: " << numBars
              << ", screenSize: " << params.screenWidth << "x" << params.screenHeight 
              << std::endl;

    // Clear any leftover GL errors before we start
    while(glGetError() != GL_NO_ERROR);

    // Update TBO data
    glBindBuffer(GL_TEXTURE_BUFFER, m_tbo);
    
    // *** ADD DEBUG PRINT HERE ***
    if (!params.bandEnergies.empty()) {
        std::cout << "DEBUG: BarGraph TBO Update - First energies: " 
                  << params.bandEnergies[0] << ", " 
                  << (params.bandEnergies.size() > 1 ? params.bandEnergies[1] : 0.0f) << ", " 
                  << (params.bandEnergies.size() > 2 ? params.bandEnergies[2] : 0.0f) << std::endl;
    }
    // ***************************
    
    size_t dataSize = numBars * sizeof(float);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, dataSize, params.bandEnergies.data());
    glBindBuffer(GL_TEXTURE_BUFFER, 0); // Unbind after update

    // Get the correct projection matrix
    glm::mat4 projection = getProjectionMatrix(params.screenWidth, params.screenHeight);

    // Calculate common uniforms
    float aspectRatio = (params.screenHeight > 0) ? static_cast<float>(params.screenWidth) / params.screenHeight : 1.0f;
    float zoomLevel = params.zoomLevel; // Use the parameter value

    // DEBUG: Print Projection Matrix
    std::cout << "BarGraph Proj Matrix (" << (m_is3D ? "3D" : "2D") << "):\n" 
              << glm::to_string(projection) << std::endl;

    if (m_is3D) {
        std::cout << "BarGraphVisualization::render - 3D Mode: Yes, numBars: " << params.bandEnergies.size() 
                  << ", screenSize: " << params.screenWidth << "x" << params.screenHeight << std::endl;
        
        std::cout << "BarGraph Proj Matrix (3D): " << std::endl << glm::to_string(m_projectionPerspective) << std::endl;
        std::cout << "Using 3D shader program ID: " << m_shader3D->getID() << std::endl;
        m_shader3D->use();
        
        err = glGetError();
        if (err != GL_NO_ERROR) std::cerr << "GL Error after shader use: " << err << std::endl;
        
        // Bind the TBO texture to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        err = glGetError();
        if (err != GL_NO_ERROR) std::cerr << "GL Error after texture bind: " << err << std::endl;
        
        // Use direct uniform setting approach with error checking
        GLint samplerLoc3D = glGetUniformLocation(m_shader3D->getID(), "bandEnergiesSampler");
        if (samplerLoc3D != -1) {
            glUniform1i(samplerLoc3D, 0);
            if (glGetError() != GL_NO_ERROR) std::cerr << "GL Error after setting 3D bandEnergiesSampler" << std::endl;
        } else {
            std::cerr << "ERROR: Could not find uniform 'bandEnergiesSampler' in 3D shader!" << std::endl;
        }
        
        GLint numBarsLoc3D = glGetUniformLocation(m_shader3D->getID(), "numBars");
        if (numBarsLoc3D != -1) {
            glUniform1f(numBarsLoc3D, static_cast<float>(numBars));
             if (glGetError() != GL_NO_ERROR) std::cerr << "GL Error after setting 3D numBars" << std::endl;
        } else {
             std::cerr << "ERROR: Could not find uniform 'numBars' in 3D shader!" << std::endl;
        }
        
        // Set remaining uniforms
        glUniform3fv(glGetUniformLocation(m_shader3D->getID(), "lowColor"), 1, glm::value_ptr(params.lowColor));
        glUniform3fv(glGetUniformLocation(m_shader3D->getID(), "midColor"), 1, glm::value_ptr(params.midColor));
        glUniform3fv(glGetUniformLocation(m_shader3D->getID(), "highColor"), 1, glm::value_ptr(params.highColor));
        glUniform1f(glGetUniformLocation(m_shader3D->getID(), "time"), params.time);
        glUniform1f(glGetUniformLocation(m_shader3D->getID(), "rotationSpeed"), params.rotationSpeed);
        
        // Calculate view matrix with camera rotation based on time and rotation speed
        float time = params.time;
        float rotationSpeed = params.rotationSpeed;
        
        // Position camera at a better angle with auto-rotation
        float radius = 1.8f;
        float camY = 1.0f;
        
        // Calculate camera position using time and rotation speed
        float angle = time * rotationSpeed;
        float camX = sin(angle) * radius;
        float camZ = cos(angle) * radius;
        
        // Position the camera and point it at the center
        glm::mat4 view = glm::lookAt(
            glm::vec3(camX, camY, camZ),    // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f),    // Look at the center (origin of the bars)
            glm::vec3(0.0f, 1.0f, 0.0f)     // Up vector
        );
        
        // Create simple identity model matrix
        glm::mat4 model = glm::mat4(1.0f);
        
        // Set transformation matrices
        glUniformMatrix4fv(glGetUniformLocation(m_shader3D->getID(), "projection"), 1, GL_FALSE, glm::value_ptr(m_projectionPerspective));
        glUniformMatrix4fv(glGetUniformLocation(m_shader3D->getID(), "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_shader3D->getID(), "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        // Final check before draw
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "GL Error before 3D draw call setup: " << err << std::endl;
        }
        
        // *** Explicitly bind texture to unit 0 ***
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        // ******************************************
        
        // Enable depth testing and face culling for 3D rendering
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        
        // Bind VAO and draw
        glBindVertexArray(m_vao3D);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, static_cast<GLsizei>(numBars));
        
        // Cleanup state
        glBindVertexArray(0);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_BUFFER, 0); // Unbind texture
        
    } else {
        // 2D rendering
        std::cout << "Using 2D shader program ID: " << m_shader2D->getID() << std::endl;
        
        // Check that shader is valid
        if (!m_shader2D || !m_shader2D->isValid()) {
            std::cerr << "2D shader is invalid or uninitialized!" << std::endl;
            return;
        }
        
        m_shader2D->use();
        err = glGetError();
        if (err != GL_NO_ERROR) std::cerr << "GL Error after 2D shader use: " << err << std::endl;
        
        // Bind the TBO texture to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, m_tboTexture);
        err = glGetError();
        if (err != GL_NO_ERROR) std::cerr << "GL Error after 2D texture bind: " << err << std::endl;
        
        // Try both uniform names for compatibility
        GLint samplerLoc = glGetUniformLocation(m_shader2D->getID(), "energyLevels");
        if (samplerLoc == -1) {
            samplerLoc = glGetUniformLocation(m_shader2D->getID(), "bandEnergiesSampler");
        }
        
        if (samplerLoc != -1) {
            glUniform1i(samplerLoc, 0);
        } else {
            std::cerr << "Error: Neither 'energyLevels' nor 'bandEnergiesSampler' uniform found in 2D shader" << std::endl;
        }
        
        // Set remaining uniforms for 2D shader
        glUniform1f(glGetUniformLocation(m_shader2D->getID(), "numBars"), static_cast<float>(numBars));
        glUniform3fv(glGetUniformLocation(m_shader2D->getID(), "lowColor"), 1, glm::value_ptr(params.lowColor));
        glUniform3fv(glGetUniformLocation(m_shader2D->getID(), "midColor"), 1, glm::value_ptr(params.midColor));
        glUniform3fv(glGetUniformLocation(m_shader2D->getID(), "highColor"), 1, glm::value_ptr(params.highColor));
        glUniform1f(glGetUniformLocation(m_shader2D->getID(), "time"), params.time);
        glUniform1f(glGetUniformLocation(m_shader2D->getID(), "zoomLevel"), zoomLevel); // Use zoom level parameter
        
        // 2D specific uniforms
        glUniform1f(glGetUniformLocation(m_shader2D->getID(), "aspectRatio"), aspectRatio);
        
        // Create model and view matrices for 2D
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        
        // Set matrices
        glUniformMatrix4fv(glGetUniformLocation(m_shader2D->getID(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(m_shader2D->getID(), "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_shader2D->getID(), "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "GL Error after setting 2D uniforms: " << err << std::endl;
        }
        
        // Draw with 2D VAO
        glBindVertexArray(m_vao2D);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(numBars));
        glBindVertexArray(0);
    }

    // Cleanup shared state
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
} 