#version 330 core
layout (location = 0) in vec3 aPos; // Base cube vertex position (-0.5 to 0.5 range)

// Outputs
out vec3 Color;

// Uniforms
uniform mat4 model; // Added model matrix
uniform mat4 view;
uniform mat4 projection;
uniform float aspectRatio; // Keep for calculating bar width
uniform float numBars;
uniform float zoomLevel;
uniform float time;
uniform vec3 lowColor;
uniform vec3 midColor;
uniform vec3 highColor;

// Texture Buffer Object for band energies
uniform samplerBuffer bandEnergiesSampler;

// Helper function to interpolate color (remains the same)
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
    // --- Calculate bar properties based on instance ID ---
    int barIndex = gl_InstanceID;
    float energy = texelFetch(bandEnergiesSampler, barIndex).r; 
    energy = clamp(energy, 0.0, 1.0); 

    float barWidth = (aspectRatio * 2.0) / numBars;
    float spacing = barWidth * 0.05;
    float totalBarWidth = barWidth - spacing;
    float startX = -aspectRatio;
    float xPos = startX + float(barIndex) * barWidth + totalBarWidth * 0.5; // Center of the bar

    // Apply wave effect and zoom
    float wave = sin(time * 2.0 + float(barIndex) * 0.1) * 0.1 + 0.9;
    float height = energy * wave * zoomLevel * 2.0; // Y-scale (0 to 2 range approx)
    height = max(height, 0.01); // Ensure a minimum height to avoid degenerate cubes
    float barDepth = totalBarWidth; // Make depth same as width for cuboid shape

    // Calculate color
    float hue = float(barIndex) / numBars;
    vec3 finalColor = interpolateColor(hue, energy, lowColor, midColor, highColor);
    Color = finalColor; // Pass color to fragment shader

    // --- Transform base cube vertex ---
    // Assume aPos is a unit cube vertex centered at origin (-0.5 to 0.5)
    vec3 scaledPos = aPos;
    scaledPos.x *= totalBarWidth;
    scaledPos.y *= height;
    scaledPos.z *= barDepth; 

    // Translate scaled vertex: Shift Y up so base is at y=0, then shift X to bar position
    vec3 finalPos = scaledPos;
    finalPos.y += height * 0.5; // Shift base to y=0
    finalPos.x += xPos;         // Shift to bar's horizontal position
    // finalPos.z can remain centered at z=0 or shifted if desired

    gl_Position = projection * view * model * vec4(finalPos, 1.0);
} 