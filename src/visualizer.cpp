#include <GL/glew.h> // Include GLEW first!
#include "visualizer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <memory> // For make_unique

// Include the concrete style implementations
#include "BarGraphVisualization.h"
#include "CircularVisualization.h"
#include "WaveVisualization.h"

Visualizer::Visualizer(AudioProcessor& audioProcessor)
    : m_audioProcessor(audioProcessor), m_style(VisualizationStyle::BAR_GRAPH),
      m_rotationSpeed(1.0f), m_zoomLevel(1.0f), m_width(0), m_height(0),
      m_VAO(0), m_VBO(0), m_shaderProgram(0),
      m_customLowColor(0.1f, 0.4f, 0.8f),
      m_customMidColor(0.0f, 0.8f, 0.6f),
      m_customHighColor(1.0f, 0.2f, 0.4f) {
    // Initialize smoothed energies moved to constructor / first render of specific style
    // m_smoothedBandEnergies.resize(m_audioProcessor.getBandEnergies().size(), 0.0f);

    // Initialize with a default style
    m_currentStylePtr = std::make_unique<BarGraphVisualization>();
}

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
    
    if (m_audioProcessor.hasLoadedFile() && !m_audioProcessor.isCurrentlyPlayingFile()) {
        return; // Skip drawing if paused
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.use();

    glViewport(0, 0, m_width, m_height);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f); // Identity view matrix for 2D
    
    float aspect = (m_height > 0) ? static_cast<float>(m_width) / m_height : 1.0f;
    glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

    m_shader.setMat4("model", model);
    m_shader.setMat4("view", view);
    m_shader.setMat4("projection", projection);

    // --- Prepare Render Parameters --- 
    RenderParameters params = {
        .bandEnergies = m_audioProcessor.getBandEnergies(),
        .audioData = m_audioProcessor.getAudioData(),
        // .smoothedEnergies = m_smoothedBandEnergies, // Removed from Visualizer
        .smoothedEnergies = {}, // Pass empty for now, styles manage their own
        .shader = m_shader,
        .vao = m_VAO,
        .vbo = m_VBO,
        .screenWidth = m_width,
        .screenHeight = m_height,
        .time = static_cast<float>(glfwGetTime()),
        .zoomLevel = m_zoomLevel,
        .rotationSpeed = m_rotationSpeed,
        .lowColor = m_customLowColor,
        .midColor = m_customMidColor,
        .highColor = m_customHighColor,
        .vizSmoothingFactor = m_smoothingFactorViz
    };

    // --- Render using the current style object --- 
    if (m_currentStylePtr) {
        m_currentStylePtr->render(params);
    } else {
        // Optionally log an error if style pointer is null
        std::cerr << "Error: No visualization style selected!" << std::endl;
    }

    glDisable(GL_BLEND);
}

void Visualizer::updateSettings(VisualizationStyle style, float rotationSpeed, float zoomLevel) {
    bool styleChanged = (m_style != style);
    m_style = style;
    m_rotationSpeed = rotationSpeed;
    m_zoomLevel = zoomLevel;

    // If the style changed, create the new style object
    if (styleChanged) {
        switch (m_style) {
            case VisualizationStyle::BAR_GRAPH:
                m_currentStylePtr = std::make_unique<BarGraphVisualization>();
                break;
            case VisualizationStyle::CIRCULAR:
                m_currentStylePtr = std::make_unique<CircularVisualization>();
                break;
            case VisualizationStyle::WAVE:
                m_currentStylePtr = std::make_unique<WaveVisualization>();
                break;
            default:
                 m_currentStylePtr = nullptr; // Or default to BarGraph?
                 std::cerr << "Error: Unknown visualization style selected in updateSettings!" << std::endl;
                break;
        }
    }
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