#pragma once

#include "IVisualizationStyle.h"
#include <vector> // Include vector for m_smoothedEnergies member
#define _USE_MATH_DEFINES // For M_PI
#include <cmath>

class CircularVisualization : public IVisualizationStyle {
public:
    CircularVisualization(); // Constructor to initialize smoothed data size
    ~CircularVisualization() override = default;

    void render(RenderParameters& params) override;

private:
    // Helper to interpolate color (could be moved to a utility class)
    glm::vec3 interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);

    // Internal state for smoothing (needs to persist between frames)
    std::vector<float> m_smoothedEnergies; 
}; 