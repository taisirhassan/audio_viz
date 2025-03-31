#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "audio_processor.h"
#include "visualizer.h"
#include <iostream>
#include <string>
#include <glm/glm.hpp>

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
    // glViewport(0, 0, width, height); // This is now handled in Visualizer::resize
    if (visualizer) { // Ensure visualizer is initialized
        visualizer->resize(width, height);
    }
}

void renderUI(GLFWwindow* window, AudioProcessor& audioProcessor, Visualizer& visualizer) {
    // Set window position and size
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

    // Create a window for controls with specific flags to make it more visible
    ImGui::Begin("Audio Visualizer Controls", nullptr, 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoCollapse);

    // Audio device selection
    if (ImGui::CollapsingHeader("Audio Device", ImGuiTreeNodeFlags_DefaultOpen)) {
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

    // Visualization controls - always visible
    ImGui::Text("Visualization");
    ImGui::Separator();
    
    static int currentStyle = static_cast<int>(VisualizationStyle::BAR_GRAPH);
    const char* styles[] = { "Bar Graph", "Circular", "Wave" };
    
    if (ImGui::Combo("Style", &currentStyle, styles, IM_ARRAYSIZE(styles))) {
        visualizer.updateSettings(static_cast<VisualizationStyle>(currentStyle),
                               visualizer.getRotationSpeed(),
                               visualizer.getZoomLevel());
    }

    ImGui::Spacing();
    float rotationSpeed = visualizer.getRotationSpeed();
    if (ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 5.0f)) {
        visualizer.updateSettings(static_cast<VisualizationStyle>(currentStyle),
                               rotationSpeed,
                               visualizer.getZoomLevel());
    }

    float zoomLevel = visualizer.getZoomLevel();
    if (ImGui::SliderFloat("Zoom Level", &zoomLevel, 0.1f, 2.0f)) {
        visualizer.updateSettings(static_cast<VisualizationStyle>(currentStyle),
                               rotationSpeed,
                               zoomLevel);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Colors");
    ImGui::Spacing();
    
    float lowColor[3] = { visualizer.getLowColor().x, visualizer.getLowColor().y, visualizer.getLowColor().z };
    float midColor[3] = { visualizer.getMidColor().x, visualizer.getMidColor().y, visualizer.getMidColor().z };
    float highColor[3] = { visualizer.getHighColor().x, visualizer.getHighColor().y, visualizer.getHighColor().z };
    
    bool colorChanged = false;
    if (ImGui::ColorEdit3("Low Frequency", lowColor)) { 
        colorChanged = true; 
    }
    if (ImGui::ColorEdit3("Mid Frequency", midColor)) { 
        colorChanged = true; 
    }
    if (ImGui::ColorEdit3("High Frequency", highColor)) { 
        colorChanged = true; 
    }
    
    if (colorChanged) {
        visualizer.updateColors(
            glm::vec3(lowColor[0], lowColor[1], lowColor[2]),
            glm::vec3(midColor[0], midColor[1], midColor[2]),
            glm::vec3(highColor[0], highColor[1], highColor[2])
        );
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
    if (ImGui::CollapsingHeader("File Playback", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool fileLoaded = audioProcessor.hasLoadedFile();

        // Always show the Open button
        if (ImGui::Button("Open Audio File")) {
            std::string filePath = openFileDialog();
            if (!filePath.empty()) {
                audioProcessor.loadAudioFile(filePath);
                fileLoaded = audioProcessor.hasLoadedFile();
            }
        }
        
        if (fileLoaded) {
            bool isCurrentlyPlaying = audioProcessor.isCurrentlyPlayingFile();
            ImGui::SameLine();
            // Show Play/Pause button
            if (ImGui::Button(isCurrentlyPlaying ? "Pause" : "Play")) {
                audioProcessor.setFilePlayback(!isCurrentlyPlaying);
            }

            ImGui::SameLine();
            // Show Stop button
            if (ImGui::Button("Stop File & Use Mic")) {
                audioProcessor.switchToInputDevice();
            }
        }
    }

    ImGui::End();
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
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Enhance control responsiveness and visibility
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.FrameBorderSize = 1.0f;
    style.GrabMinSize = 20.0f;  // Make sliders easier to grab
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.IndentSpacing = 12.0f;
    style.ScrollbarSize = 14.0f;
    style.WindowRounding = 4.0f;
    style.ChildRounding = 2.0f;
    style.PopupRounding = 2.0f;
    
    // Increase alpha to make controls more visible
    style.Alpha = 0.9f;
    style.DisabledAlpha = 0.6f;
    
    // Make active elements more visible
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.8f, 0.8f, 0.8f, 0.9f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.9f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 0.9f);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Update audio processor
        audioProcessor.processAudio();

        // Clear the background
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Define the UI elements
        renderUI(window, audioProcessor, *visualizer);

        // Render main visualization
        visualizer->render();

        // Render Dear ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
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