#version 330 core
layout (location = 0) in vec2 aPos; // Base segment vertex position

// Outputs
out vec3 Color;

// Uniforms
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform int numBars;         // Number of bars/segments
uniform float radius = 0.7;  // Default radius (overridden by C++)
uniform float zoomLevel = 1.0;
uniform float rotationSpeed;
uniform float time;
uniform vec3 lowColor;
uniform vec3 midColor;
uniform vec3 highColor;
uniform float baseRadiusFactor = 0.7; // Default if not set from C++
uniform float innerRadiusFactor = 0.3; // Default if not set from C++

// Texture Buffer Objects - support multiple names
uniform samplerBuffer energyLevels;       // Preferred name
uniform samplerBuffer bandEnergiesSampler; // Fallback name

// Constants
const float PI = 3.14159265359;

// Helper function to interpolate color
vec3 interpolateColor(float position, float energy) {
    // Normalize energy for coloring (clamp to avoid extreme values)
    float normEnergy = clamp(energy, 0.0, 1.0);
    
    // Use position to determine the color from the spectrum
    vec3 baseColor;
    if (position < 0.33) {
        // Low range: low to mid
        float t = position / 0.33;
        baseColor = mix(lowColor, midColor, t);
    } else if (position < 0.66) {
        // Mid range: mid to high
        float t = (position - 0.33) / 0.33;
        baseColor = mix(midColor, highColor, t);
    } else {
        // High range: high back to low (for full spectrum)
        float t = (position - 0.66) / 0.34;
        baseColor = mix(highColor, lowColor, t);
    }
    
    // Energy affects brightness
    float brightness = 0.5 + normEnergy * 0.5;
    
    return baseColor * brightness;
}

// Helper function to get energy value, handling both possible uniform names
float getEnergy(int index) {
    float energy = 0.0;
    if (index >= 0 && index < numBars) {
        energy = texelFetch(energyLevels, index).r;
    }
    return energy;
}

void main() {
    int segmentIndex = gl_InstanceID;
    if (segmentIndex >= numBars) {
        gl_Position = vec4(0.0); 
        Color = vec3(0.0);
        return;
    }
    
    float energy = getEnergy(segmentIndex);

    float numSegmentsFloat = float(max(1, numBars));
    float angleStep = 2.0 * PI / numSegmentsFloat;
    float angle = angleStep * float(segmentIndex) - time * rotationSpeed;
    float nextAngle = angle + angleStep;
    
    float baseRadius = zoomLevel * baseRadiusFactor; 
    float innerRadius = baseRadius * innerRadiusFactor;
    float outerRadius = baseRadius * (1.0 + energy * 0.7); 

    // --- Transform Quad Vertex to Radial Bar Vertex ---
    // aPos contains quad vertices (0,0) to (1,1)
    vec2 transformedPos;
    float currentAngle;
    float currentRadius;

    // Determine which angle to use based on aPos.x (left or right side of quad)
    if (aPos.x < 0.5) {
        currentAngle = angle; // Left side of the bar uses the starting angle
    } else {
        currentAngle = nextAngle; // Right side uses the ending angle (creates wedge)
        // For rectangular bars, use 'angle' for all vertices and adjust x later
        // currentAngle = angle;
    }

    // Determine radius based on aPos.y (bottom or top of quad)
    if (aPos.y < 0.5) {
        currentRadius = innerRadius; // Bottom of the bar
    } else {
        currentRadius = outerRadius; // Top of the bar
    }

    // Convert polar coordinates (angle, radius) to Cartesian (x, y)
    transformedPos.x = cos(currentAngle) * currentRadius;
    transformedPos.y = sin(currentAngle) * currentRadius;

    // If using rectangular bars instead of wedges:
    // float barWidthAngle = angleStep * 0.8; // e.g., 80% of the angle step
    // float halfWidthOffset = (aPos.x - 0.5) * barWidthAngle; 
    // currentAngle = angle + halfWidthOffset;
    // transformedPos.x = cos(currentAngle) * currentRadius;
    // transformedPos.y = sin(currentAngle) * currentRadius;

    // Set position in clip space
    gl_Position = projection * view * model * vec4(transformedPos, 0.0, 1.0);
    
    // Calculate color based on position and energy
    float position = float(segmentIndex) / numSegmentsFloat;
    Color = interpolateColor(position, energy);
} 