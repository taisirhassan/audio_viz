#pragma once

#include "IVisualizationStyle.h"
#include <vector>
#include <cmath>

class BarGraphVisualization : public IVisualizationStyle {
public:
    BarGraphVisualization();
    ~BarGraphVisualization() override = default;

    void render(RenderParameters& params) override;

private:
    // Helper to interpolate color (could be moved to a utility class)
    glm::vec3 interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);

    // Internal state for smoothing (if we decide to add it later)
    // std::vector<float> m_smoothedEnergies;
}; 