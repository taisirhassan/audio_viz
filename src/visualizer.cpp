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
      m_currentStyleEnum(VisualizationStyle::BAR_GRAPH),
      m_currentStylePtr(nullptr),
      m_view(glm::mat4(1.0f)),
      m_cameraPos(0.0f, 2.0f, 5.0f),  // Position camera more directly in front
      m_cameraFront(glm::normalize(glm::vec3(0.0f, -0.2f, -1.0f))),  // Look at origin with slight downward angle
      m_cameraUp(0.0f, 1.0f, 0.0f),
      m_yaw(-90.0f),  // Straight ahead
      m_pitch(-12.0f),  // Slight downward angle
      m_rotationSpeed(1.0f),
      m_zoomLevel(1.0f),
      m_customLowColor(0.0f, 0.0f, 1.0f),
      m_customMidColor(0.0f, 1.0f, 0.0f),
      m_customHighColor(1.0f, 0.0f, 0.0f),
      m_width(0),
      m_height(0),
      m_isInitialized(false)
{
    std::cout << "Visualizer Constructor Called" << std::endl;
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
        std::cerr << "Visualizer::render called before initialization or no style set." << std::endl;
        return;
    }

    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get audio data
    const auto& bandEnergies = m_audioProcessor.getBandEnergies(); 
    const auto& audioData = m_audioProcessor.getAudioData(); // Need raw audio for Wave style

    // DEBUG: Check audio data sizes
    std::cout << "Visualizer::render - BandEnergies size: " << bandEnergies.size() 
              << ", AudioData size: " << audioData.size() << std::endl;

    // Update camera view matrix (for 3D styles)
    updateCameraView();

    // DEBUG: Print View Matrix
    std::cout << "Visualizer View Matrix:\n" << glm::to_string(m_view) << std::endl;

    // Prepare render parameters using direct initialization for references
    RenderParameters params = { 
        bandEnergies,            // bandEnergies (const reference)
        audioData,               // audioData (const reference)
        m_width,                 // screenWidth
        m_height,                // screenHeight
        static_cast<float>(glfwGetTime()), // time
        m_zoomLevel,             // zoomLevel
        m_rotationSpeed,         // rotationSpeed
        m_customLowColor,        // lowColor
        m_customMidColor,        // midColor
        m_customHighColor,       // highColor
        0.1f,                    // vizSmoothingFactor (example value)
        glm::mat4(1.0f)          // default to identity view matrix
    };
    
    // Use appropriate view matrix for the visualization style
    if (m_currentStyleEnum == VisualizationStyle::BAR_GRAPH) {
        // Only use camera view for 3D Bar Graph
        BarGraphVisualization* barGraphPtr = dynamic_cast<BarGraphVisualization*>(m_currentStylePtr);
        if (barGraphPtr && barGraphPtr->is3DMode()) {
            params.viewMatrix = m_view; // Assign the calculated 3D view matrix
            std::cout << "Visualizer View Matrix (3D Bar Graph): " << std::endl << glm::to_string(m_view) << std::endl;
        } // Otherwise, keep the identity matrix from initialization
    }
    // For other styles (Circular, Wave), the identity matrix is already set
    
    // Render the current style with parameters
    std::cout << "Visualizer calling style render..." << std::endl;
    if (m_currentStylePtr) {
        m_currentStylePtr->render(params); // Pass all parameters
    }

    // Check for GL errors after rendering
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error after Visualizer::render: " << err << std::endl;
    }
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
    if (width <= 0 || height <= 0) return; 

    m_width = width;
    m_height = height;
    glViewport(0, 0, m_width, m_height);

    // Notify all styles of the resize
    for (auto const& [styleEnum, stylePtr] : m_visualizationStyles) {
        if (stylePtr) {
            stylePtr->resize(width, height);
        }
    }
}

// --- Camera Handling ---

void Visualizer::updateCameraView() {
    // FPS-style camera view calculation
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_cameraFront = glm::normalize(front);
    // Use m_cameraPos and m_cameraFront (no m_cameraTarget)
    m_view = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);
}

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

// --- Specific Bar Graph Control ---
void Visualizer::setBarGraph3DMode(bool is3D) {
    // Only attempt if the current style is Bar Graph
    if (m_currentStyleEnum == VisualizationStyle::BAR_GRAPH && m_currentStylePtr) {
        // Safely cast the base pointer to the derived type
        BarGraphVisualization* barGraph = dynamic_cast<BarGraphVisualization*>(m_currentStylePtr);
        if (barGraph) { // Check if cast was successful
            barGraph->set3DMode(is3D);
        } else {
            std::cerr << "Error: Failed to cast current style pointer to BarGraphVisualization!" << std::endl;
        }
    }
}