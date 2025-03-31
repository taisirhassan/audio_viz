// Define before *any* includes if using experimental GLM headers anywhere
#define GLM_ENABLE_EXPERIMENTAL

#include <GL/glew.h> // Include GLEW first!
#include "visualizer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <memory> // For make_unique
#include <stdexcept> // For std::runtime_error
#include <glm/gtx/rotate_vector.hpp> // For rotating camera position

// Include the concrete style implementations
#include "BarGraphVisualization.h"
#include "CircularVisualization.h"
#include "WaveVisualization.h"

Visualizer::Visualizer(AudioProcessor& audioProcessor)
    : m_audioProcessor(audioProcessor),
      m_visualizationStyles(),
      m_currentStylePtr(nullptr),
      m_currentStyleEnum(VisualizationStyle::BAR_GRAPH),
      m_rotationSpeed(1.0f),
      m_zoomLevel(1.0f),
      m_width(0), m_height(0),
      m_projectionMatrix(),
      m_viewMatrix(),
      m_cameraTarget(0.0f, 0.0f, 0.0f),
      m_cameraUp(0.0f, 1.0f, 0.0f),
      m_cameraRadius(3.0f),
      m_cameraFov(45.0f),
      m_customLowColor(0.0f, 0.0f, 1.0f),  // Explicitly initialize blue
      m_customMidColor(0.0f, 1.0f, 0.0f),  // Explicitly initialize green
      m_customHighColor(1.0f, 0.0f, 0.0f), // Explicitly initialize red
      m_isInitialized(false)
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
    std::cout << "Visualizer::initialize START" << std::endl;
    if (m_isInitialized) {
        std::cout << "Visualizer already initialized." << std::endl;
        return true; // Already initialized
    }
    m_isInitialized = false; // Ensure it's false at start
    m_width = width;
    m_height = height;

    try {
        // Create and initialize all visualization styles
        m_visualizationStyles[VisualizationStyle::BAR_GRAPH] = std::make_unique<BarGraphVisualization>();
        m_visualizationStyles[VisualizationStyle::CIRCULAR] = std::make_unique<CircularVisualization>();
        m_visualizationStyles[VisualizationStyle::WAVE] = std::make_unique<WaveVisualization>();

        // Initialize all created styles
        for (auto const& [key, val] : m_visualizationStyles) {
            // Log which style is being initialized
            std::string styleName = "Unknown";
            switch(key) {
                case VisualizationStyle::BAR_GRAPH: styleName = "Bar Graph"; break;
                case VisualizationStyle::CIRCULAR:  styleName = "Circular"; break;
                case VisualizationStyle::WAVE:      styleName = "Wave"; break;
            }
            std::cout << "  Initializing style: " << styleName << "..." << std::endl;
            
            if (val) val->init(); // Check if val is not null before calling init
        }

        // Set the initial active style
        // Ensure the initial style actually exists in the map now
        if (m_visualizationStyles.count(initialStyle)) {
             setStyle(initialStyle);
        } else {
             std::cerr << "Warning: Initial style not available, defaulting." << std::endl;
             // Default to the first available style if the requested one isn't there
             if (!m_visualizationStyles.empty()) {
                 setStyle(m_visualizationStyles.begin()->first);
             } else {
                 std::cerr << "Error: No visualization styles available!" << std::endl;
                 m_isInitialized = false;
                 return false; // Cannot proceed without any styles
             }
        }

        resize(width, height); // Call resize to set initial projection
        // Initialize camera target and up vector
        m_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); 
        m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); 
        // Initial view matrix (will be updated in render)
        // updateCameraView(); // REMOVED CALL - View matrix calculated in render() now

    } catch (const std::exception& e) {
        std::cerr << "Error initializing Visualizer or its styles: " << e.what() << std::endl;
        // Cleanup partially initialized styles
        for (auto const& [key, val] : m_visualizationStyles) {
            if (val) val->cleanup();
        }
        m_visualizationStyles.clear();
        m_isInitialized = false; // Ensure remains false on error
        return false;
    }

    m_isInitialized = true; // Set flag to true ONLY on successful completion
    std::cout << "Visualizer::initialize SUCCESS" << std::endl;
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
    if (!m_isInitialized || !m_currentStylePtr) {
        return; // No active style or not initialized
    }

    // --- Update View Matrix for Auto-Rotation ---
    // Calculate current rotation angle based on time and speed
    float currentTime = static_cast<float>(glfwGetTime());
    float angle = currentTime * m_rotationSpeed * 30.0f; // Adjust multiplier for desired speed

    // Calculate camera position for orbiting around Y axis
    glm::vec3 cameraPos;
    cameraPos.x = sin(glm::radians(angle)) * m_cameraRadius;
    cameraPos.y = 0.0f; // Keep camera level
    cameraPos.z = cos(glm::radians(angle)) * m_cameraRadius;

    m_viewMatrix = glm::lookAt(cameraPos, m_cameraTarget, m_cameraUp);
    // --- End View Matrix Update ---

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
        m_viewMatrix, // Use the updated view matrix
        m_projectionMatrix
    };

    // DEBUG: Check if we have energy data
    if (params.bandEnergies.empty()) {
        std::cout << "DEBUG: Visualizer::render - bandEnergies is EMPTY!" << std::endl;
    } else {
        // std::cout << "DEBUG: Visualizer::render - bandEnergies size: " << params.bandEnergies.size() << std::endl; // Optional: Log size
    }

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

    // Update projection matrix for 3D Perspective
    float aspectRatio = static_cast<float>(width) / height;
    m_projectionMatrix = glm::perspective(glm::radians(m_cameraFov), // Use FOV member
                                            aspectRatio,        
                                            0.1f,               
                                            100.0f);            
}

// --- Camera Control Handlers ---

// NOTE: Mouse controls are now disabled as auto-rotation is active
void Visualizer::handleMouseButton(int /*button*/, int /*action*/, int /* mods */) {
    // (Commented out to disable manual camera control)
    /*
    if (!m_isInitialized) return; // Check init flag
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            m_isDragging = true;
            m_firstMouse = true; // Reset first mouse on new drag
        } else if (action == GLFW_RELEASE) {
            m_isDragging = false;
        }
    }
    */
}

void Visualizer::handleMouseMove(double /*xpos*/, double /*ypos*/) {
    // (Commented out to disable manual camera control)
    /*
    if (!m_isInitialized) return; // Check init flag
    if (!m_isDragging) {
        // Update last position even if not dragging to prevent jump on next drag
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouse = true; // Ensure firstMouse logic runs if dragging starts
        return;
    }

    if (m_firstMouse) {
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouse = false;
    }

    float xoffset = xpos - m_lastMouseX;
    float yoffset = m_lastMouseY - ypos; // Reversed since y-coordinates go from bottom to top
    m_lastMouseX = xpos;
    m_lastMouseY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_cameraYaw += xoffset;
    m_cameraPitch += yoffset;

    // Clamp pitch
    if (m_cameraPitch > 89.0f) m_cameraPitch = 89.0f;
    if (m_cameraPitch < -89.0f) m_cameraPitch = -89.0f;

    // Yaw wraps around naturally with cosine/sine
    // Update camera view based on new angles
    // updateCameraView(); // Already removed
    */
}

void Visualizer::handleMouseScroll(double /* xoffset */, double /*yoffset*/) {
    // (Commented out to disable manual camera control)
    /*
    if (!m_isInitialized) return; // Check init flag
    // Use vertical scroll (yoffset) to adjust radius (zoom)
    float scrollSensitivity = 0.5f; 
    m_cameraRadius -= yoffset * scrollSensitivity;

    // Clamp radius to prevent going inside the target or too far away
    if (m_cameraRadius < 0.5f) m_cameraRadius = 0.5f;
    if (m_cameraRadius > 20.0f) m_cameraRadius = 20.0f; // Add a max radius
    
    // Update camera view based on new radius
    // updateCameraView(); // Already removed
    */
}