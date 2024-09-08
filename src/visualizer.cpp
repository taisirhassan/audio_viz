#include "visualizer.h"
#include <iostream>

Visualizer::Visualizer(AudioProcessor& audioProcessor)
    : m_audioProcessor(audioProcessor), m_style(VisualizationStyle::BAR_GRAPH),
      m_rotationSpeed(1.0f), m_zoomLevel(1.0f), m_width(0), m_height(0),
      m_lowColor(0.0f, 0.0f, 1.0f), m_midColor(0.0f, 1.0f, 0.0f), m_highColor(1.0f, 0.0f, 0.0f) {}

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

    m_shader.use();

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)m_width / (float)m_height, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_shader.setMat4("projection", projection);
    m_shader.setMat4("view", view);

    switch (m_style) {
        case VisualizationStyle::BAR_GRAPH:
            renderBarGraph();
            break;
        case VisualizationStyle::CIRCULAR:
            renderCircular();
            break;
        case VisualizationStyle::WAVE:
            renderWave();
            break;
    }
}

void Visualizer::updateSettings(VisualizationStyle style, float rotationSpeed, float zoomLevel) {
    m_style = style;
    m_rotationSpeed = rotationSpeed;
    m_zoomLevel = zoomLevel;
}

void Visualizer::renderBarGraph() {
    const std::vector<float>& bandEnergies = m_audioProcessor.getBandEnergies();
    std::vector<float> vertices;

    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float x = (float)i / bandEnergies.size() * 2.0f - 1.0f;
        float y = bandEnergies[i] * m_zoomLevel;
        glm::vec3 color = getColor((float)i / bandEnergies.size());
        vertices.insert(vertices.end(), {x, 0.0f, 0.0f, color.r, color.g, color.b});
        vertices.insert(vertices.end(), {x, y, 0.0f, color.r, color.g, color.b});
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glm::mat4 model = glm::mat4(1.0f);
    m_shader.setMat4("model", model);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINES, 0, vertices.size() / 6);
}

void Visualizer::renderCircular() {
    const std::vector<float>& bandEnergies = m_audioProcessor.getBandEnergies();
    std::vector<float> vertices;

    for (size_t i = 0; i < bandEnergies.size(); i++) {
        float angle = (float)i / bandEnergies.size() * 2.0f * M_PI;
        float r = 0.5f + bandEnergies[i] * m_zoomLevel * 0.5f;
        float x = cos(angle) * r;
        float y = sin(angle) * r;
        glm::vec3 color = getColor((float)i / bandEnergies.size());
        vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f, color.r, color.g, color.b});
        vertices.insert(vertices.end(), {x, y, 0.0f, color.r, color.g, color.b});
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime() * m_rotationSpeed, glm::vec3(0.0f, 0.0f, 1.0f));
    m_shader.setMat4("model", model);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINES, 0, vertices.size() / 6);
}

void Visualizer::renderWave() {
    const std::vector<float>& audioData = m_audioProcessor.getAudioData();
    std::vector<float> vertices;

    for (size_t i = 0; i < audioData.size(); i += 2) {  // Assuming stereo audio
        float x = (float)i / audioData.size() * 2.0f - 1.0f;
        float y = audioData[i] * m_zoomLevel;
        glm::vec3 color = getColor((float)i / audioData.size());
        vertices.insert(vertices.end(), {x, y, 0.0f, color.r, color.g, color.b});
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glm::mat4 model = glm::mat4(1.0f);
    m_shader.setMat4("model", model);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 6);
}

glm::vec3 Visualizer::getColor(float t) {
    if (t < 0.5f) {
        return glm::mix(m_lowColor, m_midColor, t * 2.0f);
    } else {
        return glm::mix(m_midColor, m_highColor, (t - 0.5f) * 2.0f);
    }
}