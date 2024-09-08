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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
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

    VisualizationStyle style = VisualizationStyle::BAR_GRAPH;
    float rotationSpeed = 1.0f;
    float zoomLevel = 1.0f;
    bool isPlayingFile = false;
    std::string audioFilePath;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create ImGui window
        ImGui::Begin("Audio Visualizer Controls");
        
        if (ImGui::RadioButton("Bar Graph", style == VisualizationStyle::BAR_GRAPH)) style = VisualizationStyle::BAR_GRAPH;
        ImGui::SameLine();
        if (ImGui::RadioButton("Circular", style == VisualizationStyle::CIRCULAR)) style = VisualizationStyle::CIRCULAR;
        ImGui::SameLine();
        if (ImGui::RadioButton("Wave", style == VisualizationStyle::WAVE)) style = VisualizationStyle::WAVE;

        ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 5.0f);
        ImGui::SliderFloat("Zoom Level", &zoomLevel, 0.1f, 3.0f);

        if (ImGui::Button("Toggle Audio Source")) {
            audioProcessor.toggleAudioSource();
            isPlayingFile = !isPlayingFile;
        }

        ImGui::SameLine();
        ImGui::Text(isPlayingFile ? "Playing File" : "Live Input");

        if (ImGui::Button("Load Audio File")) {
            std::string filePath = openFileDialog();
            if (!filePath.empty()) {
                if (audioProcessor.loadAudioFile(filePath)) {
                    std::cout << "Audio file loaded successfully: " << filePath << std::endl;
                    audioFilePath = filePath;
                    isPlayingFile = true;
                    audioProcessor.toggleAudioSource();  // Switch to file playback
                } else {
                    std::cerr << "Failed to load audio file: " << filePath << std::endl;
                }
            }
        }

        if (!audioFilePath.empty()) {
            ImGui::Text("Loaded file: %s", audioFilePath.c_str());
        }

        ImGui::End();

        // Update audio processor and visualizer
        audioProcessor.processAudio();
        visualizer->updateSettings(style, rotationSpeed, zoomLevel);

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        visualizer->render();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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