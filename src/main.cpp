#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "audio_processor.h"
#include "visualizer.h"
#include <iostream>
#include <string>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 1024;
const int NUM_CHANNELS = 2;

AudioProcessor audioProcessor;
Visualizer* visualizer;

// Declare the openFileDialog function (implemented in file_dialog_mac.mm)
std::string openFileDialog();

void framebuffer_size_callback([[maybe_unused]] GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void renderUI([[maybe_unused]] GLFWwindow* window, AudioProcessor& audioProcessor, Visualizer& visualizer) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create a window for controls
    ImGui::Begin("Audio Visualizer Controls");

    // Audio device selection
    if (ImGui::CollapsingHeader("Audio Device")) {
        const auto& devices = audioProcessor.getInputDevices();
        static int currentDevice = 0;
        
        if (ImGui::BeginCombo("Input Device", devices[currentDevice].c_str())) {
            for (size_t i = 0; i < devices.size(); i++) {
                bool isSelected = (currentDevice == static_cast<int>(i));
                if (ImGui::Selectable(devices[i].c_str(), isSelected)) {
                    currentDevice = static_cast<int>(i);
                    audioProcessor.setInputDevice(currentDevice);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    // Visualization style selection
    if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int currentStyle = static_cast<int>(VisualizationStyle::BAR_GRAPH);
        const char* styles[] = { "Bar Graph", "Circular", "Wave" };
        
        if (ImGui::Combo("Style", &currentStyle, styles, IM_ARRAYSIZE(styles))) {
            visualizer.updateSettings(static_cast<VisualizationStyle>(currentStyle),
                                   visualizer.getRotationSpeed(),
                                   visualizer.getZoomLevel());
        }

        // Add sliders for visualization parameters
        float rotationSpeed = visualizer.getRotationSpeed();
        if (ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 5.0f)) {
            visualizer.updateSettings(static_cast<VisualizationStyle>(currentStyle),
                                   rotationSpeed,
                                   visualizer.getZoomLevel());
        }

        float zoomLevel = visualizer.getZoomLevel();
        if (ImGui::SliderFloat("Zoom Level", &zoomLevel, 0.1f, 2.0f)) {
            visualizer.updateSettings(static_cast<VisualizationStyle>(currentStyle),
                                   visualizer.getRotationSpeed(),
                                   zoomLevel);
        }
    }

    // Audio processing parameters
    if (ImGui::CollapsingHeader("Audio Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
        float smoothing = audioProcessor.getSmoothingFactor();
        if (ImGui::SliderFloat("Smoothing", &smoothing, 0.0f, 0.95f)) {
            audioProcessor.setSmoothingFactor(smoothing);
        }
        ImGui::SameLine(); 
        if (ImGui::Button("Reset##1")) {
            audioProcessor.setSmoothingFactor(0.3f);
        }

        float normalization = audioProcessor.getNormalizationFactor();
        if (ImGui::SliderFloat("Gain", &normalization, 0.1f, 20.0f)) {
            audioProcessor.setNormalizationFactor(normalization);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset##2")) {
            audioProcessor.setNormalizationFactor(5.0f);
        }
    }

    // File playback controls
    if (ImGui::CollapsingHeader("File Playback")) {
        static bool isPlaying = false;  // Use this instead of isPlayingFile
        
        if (ImGui::Button("Open Audio File")) {
            std::string filePath = openFileDialog();
            if (!filePath.empty()) {
                audioProcessor.loadAudioFile(filePath);
                isPlaying = true;
            }
        }
        
        if (audioProcessor.hasLoadedFile()) {
            if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
                isPlaying = !isPlaying;
                audioProcessor.setFilePlayback(isPlaying);
            }
        }
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Audio Visualizer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Initialize audio processor
    if (!audioProcessor.initialize(SAMPLE_RATE, FRAMES_PER_BUFFER, NUM_CHANNELS)) {
        std::cerr << "Failed to initialize audio processor" << std::endl;
        return -1;
    }

    // Initialize visualizer
    visualizer = new Visualizer(audioProcessor);
    if (!visualizer->initialize(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        std::cerr << "Failed to initialize visualizer" << std::endl;
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        renderUI(window, audioProcessor, *visualizer);

        // Update audio processor
        audioProcessor.processAudio();
        // Visualizer settings are now updated via UI callbacks
        // visualizer->updateSettings(style, rotationSpeed, zoomLevel); // Removed

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        visualizer->render();

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete visualizer;
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}