#version 330 core
layout (location = 0) in vec2 aPos; // Base segment vertex position (e.g., 0 or 1 along a line)

// Outputs
out vec3 Color;

// Uniforms
uniform mat4 view;
uniform mat4 projection;
uniform float numSegments;
uniform float zoomLevel;
uniform float rotationSpeed;
uniform float time;
uniform vec3 lowColor;
uniform vec3 midColor;
uniform vec3 highColor;
uniform float baseRadiusFactor; // e.g., 0.7, passed from C++
uniform float innerRadiusFactor; // e.g., 0.3, passed from C++

// Texture Buffer Object for smoothed band energies
uniform samplerBuffer smoothedEnergiesSampler;

// Constants
const float PI = 3.14159265359;

// Helper function to interpolate color (same as bar graph shader)
vec3 interpolateColor(float hue, float energy, vec3 lowC, vec3 midC, vec3 highC) {
    vec3 color;
    float hueNormalized = mod(hue + energy * 0.2, 1.0);
    if (hueNormalized < 0.33) {
        color = mix(lowC, midC, hueNormalized / 0.33);
    } else if (hueNormalized < 0.66) {
        color = mix(midC, highC, (hueNormalized - 0.33) / 0.33);
    } else {
        color = mix(highC, lowC, (hueNormalized - 0.66) / 0.33);
    }
    float brightness = 0.5 + energy * 0.5;
    return color * brightness;
}

void main() {
    // --- Calculate segment properties based on instance ID ---
    int segmentIndex = gl_InstanceID;
    float smoothedEnergy = texelFetch(smoothedEnergiesSampler, segmentIndex).r;
    // Don't clamp energy here as it might represent smoothed state > 1 momentarily

    float angleStep = 2.0 * PI / numSegments;
    float angle = angleStep * float(segmentIndex) - time * rotationSpeed;
    float nextAngle = angleStep * float(segmentIndex + 1) - time * rotationSpeed;

    // Calculate radii
    float baseRadius = baseRadiusFactor * zoomLevel;
    float innerRadius = baseRadius * innerRadiusFactor;

    float wave = sin(time * 1.5 + float(segmentIndex) * 0.1) * 0.08 + 0.92;
    float currentOuterRadius = baseRadius * (1.0 + smoothedEnergy * wave * 0.6);
    // Add clamping if needed, maybe based on aspect ratio passed as uniform
    // currentOuterRadius = min(currentOuterRadius, some_max_radius);

    // Calculate color (normalize energy for color calculation)
    float colorEnergy = clamp(smoothedEnergy / (1.2 * zoomLevel), 0.0, 1.0);
    float hue = float(segmentIndex) / numSegments;
    vec3 finalColor = interpolateColor(hue, colorEnergy, lowColor, midColor, highColor);
    
    // --- Determine vertex position based on aPos and segment geometry ---
    // We draw quads (4 vertices per instance: inner1, inner2, outer2, outer1)
    // Let's assume aPos.x indicates which vertex (0, 1, 2, 3) 
    // and aPos.y is unused or could indicate inner/outer (0/1)

    float currentAngle;
    float currentRadius;
    vec3 segmentColor;

    // Use integer index from aPos.x (requires specific VBO setup)
    // Or, more simply, use a 4-vertex base quad (0,0), (1,0), (1,1), (0,1)
    // and interpret aPos.x for angle (0->angle, 1->nextAngle)
    // and aPos.y for radius (0->innerRadius, 1->outerRadius)

    if (aPos.x < 0.5) { // Left vertices (angle)
        currentAngle = angle;
    } else { // Right vertices (nextAngle)
        currentAngle = nextAngle;
    }

    if (aPos.y < 0.5) { // Bottom vertices (inner radius)
        currentRadius = innerRadius;
        segmentColor = finalColor * 0.3; // Darker inner color
    } else { // Top vertices (outer radius)
        currentRadius = currentOuterRadius;
        segmentColor = finalColor; // Full outer color
    }

    Color = segmentColor; // Pass color to fragment shader

    vec2 posXY;
    posXY.x = cos(currentAngle) * currentRadius;
    posXY.y = sin(currentAngle) * currentRadius;

    gl_Position = projection * view * vec4(posXY, 0.0, 1.0);
} 