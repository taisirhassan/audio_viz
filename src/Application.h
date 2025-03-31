#pragma once

// Forward declarations
struct GLFWwindow;
class AudioProcessor;
class Visualizer;
class UIManager;

class Application {
public:
    Application();
    ~Application();

    // Initializes all components and returns true on success
    bool initialize();

    // Runs the main application loop
    void run();

    // Cleanup function
    void cleanup();

private:
    // Private helper for initialization steps
    bool initGLFW();
    bool initGLEW();
    bool initImGui();
    bool initModules(); // Initializes AudioProcessor, Visualizer, UIManager

    // Callback function - needs to be static or global
    // We'll pass 'this' Application instance via glfwSetWindowUserPointer
    static void framebuffer_size_callback_static(GLFWwindow* window, int width, int height);
    void framebuffer_size_callback(int width, int height); // Instance method

    // Static callback functions (wrappers)
    static void cursor_position_callback_static(GLFWwindow* window, double xpos, double ypos);
    static void mouse_button_callback_static(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback_static(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* m_window = nullptr;
    AudioProcessor* m_audioProcessor = nullptr;
    Visualizer* m_visualizer = nullptr;
    UIManager* m_uiManager = nullptr;

    // Constants (could also be passed to constructor or read from config)
    const int WINDOW_WIDTH = 1280; // Let's default to a slightly larger size
    const int WINDOW_HEIGHT = 720;
    const int SAMPLE_RATE = 44100;
    const int FRAMES_PER_BUFFER = 1024;
    const int NUM_CHANNELS = 2; // Default preferred channels
}; 