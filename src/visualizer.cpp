#include <GL/glew.h> // Include GLEW first!
#include "visualizer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <memory> // For make_unique
#include <stdexcept> // For std::runtime_error

// Include the concrete style implementations
#include "BarGraphVisualization.h"
#include "CircularVisualization.h"
#include "WaveVisualization.h"

Visualizer::Visualizer(AudioProcessor& audioProcessor)
    : m_audioProcessor(audioProcessor),
      m_currentStylePtr(nullptr),
      m_currentStyleEnum(VisualizationStyle::BAR_GRAPH), // Default style
      m_width(0), m_height(0),
      m_rotationSpeed(1.0f), m_zoomLevel(1.0f)
{
    // Default colors are set in the header initializer list now
}

Visualizer::~Visualizer() {
    // Explicitly cleanup styles before unique_ptrs are destroyed
    for (auto const& [key, val] : m_visualizationStyles) {
        if (val) { // Check if pointer is valid
            val->cleanup();
        }
    }
    m_visualizationStyles.clear(); // Clear the map
    m_currentStylePtr = nullptr;
}

// Initialize Visualizer: Create all styles, init them, set initial style
bool Visualizer::initialize(int width, int height, VisualizationStyle initialStyle) {
    m_width = width;
    m_height = height;

    try {
        // Create and initialize all visualization styles
        m_visualizationStyles[VisualizationStyle::BAR_GRAPH] = std::make_unique<BarGraphVisualization>();
        m_visualizationStyles[VisualizationStyle::CIRCULAR] = std::make_unique<CircularVisualization>();
        m_visualizationStyles[VisualizationStyle::WAVE] = std::make_unique<WaveVisualization>();

        for (auto const& [key, val] : m_visualizationStyles) {
            val->init(); // Call init on each style
        }

        // Set the initial active style
        setStyle(initialStyle);

        // Initialize matrices
        resize(width, height); // Call resize to set initial projection
        m_viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), // Camera position
                                 glm::vec3(0.0f, 0.0f, 0.0f), // Look at origin
                                 glm::vec3(0.0f, 1.0f, 0.0f)); // Up vector

    } catch (const std::exception& e) {
        std::cerr << "Error initializing Visualizer or its styles: " << e.what() << std::endl;
        // Cleanup partially initialized styles
        for (auto const& [key, val] : m_visualizationStyles) {
            if (val) val->cleanup();
        }
        m_visualizationStyles.clear();
        return false;
    }

    return true;
}

// Set the current active visualization style
void Visualizer::setStyle(VisualizationStyle newStyle) {
    if (m_visualizationStyles.count(newStyle)) {
        m_currentStylePtr = m_visualizationStyles[newStyle].get();
        m_currentStyleEnum = newStyle;
    } else {
        std::cerr << "Warning: Attempted to set an unknown visualization style." << std::endl;
        // Optionally default to a known style or handle error
        if (!m_visualizationStyles.empty()) {
             m_currentStylePtr = m_visualizationStyles.begin()->second.get();
             m_currentStyleEnum = m_visualizationStyles.begin()->first;
        }
    }
}

void Visualizer::render() {
    if (!m_currentStylePtr) {
        return; // No active style
    }

    // Prepare render parameters
    RenderParameters params = {
        m_audioProcessor.getBandEnergies(),
        m_audioProcessor.getAudioData(),
        m_width,
        m_height,
        static_cast<float>(glfwGetTime()),
        m_zoomLevel,
        m_rotationSpeed,
        m_customLowColor,
        m_customMidColor,
        m_customHighColor,
        m_smoothingFactorViz,
        m_viewMatrix,
        m_projectionMatrix
    };

    // Call the render method of the current visualization style
    m_currentStylePtr->render(params);
}

void Visualizer::updateSettings(VisualizationStyle style, float rotationSpeed, float zoomLevel) {
    if (style != m_currentStyleEnum) {
        setStyle(style);
    }
    m_rotationSpeed = rotationSpeed;
    m_zoomLevel = zoomLevel;
}

void Visualizer::updateColors(const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high) {
    m_customLowColor = low;
    m_customMidColor = mid;
    m_customHighColor = high;
}

void Visualizer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    if (height == 0) height = 1; // Prevent division by zero

    glViewport(0, 0, width, height);

    // Update projection matrix
    float aspectRatio = static_cast<float>(width) / height;
    // Simple orthographic projection for 2D visualizations, adjust as needed for 3D
    // Assuming coordinates range from -aspectRatio to +aspectRatio horizontally,
    // and -1 to +1 vertically.
    m_projectionMatrix = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 100.0f); 
    
    // If using perspective for 3D:
    // m_projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
}