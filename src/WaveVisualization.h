#pragma once

#include "IVisualizationStyle.h"
#include <cmath>

class WaveVisualization : public IVisualizationStyle {
public:
    WaveVisualization() = default;
    ~WaveVisualization() override = default;

    void render(RenderParameters& params) override;

private:
    // Helper to interpolate color
    glm::vec3 interpolateColor(float hue, float energy, const glm::vec3& low, const glm::vec3& mid, const glm::vec3& high);
}; 