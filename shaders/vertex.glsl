#version 330 core
layout (location = 0) in vec2 aPos; // Base quad vertex position (0-1 range)

// Outputs
out vec3 Color;

// Uniforms
uniform mat4 view;
uniform mat4 projection;
uniform float aspectRatio;
uniform float numBars;
uniform float zoomLevel;
uniform float time;
uniform vec3 lowColor;
uniform vec3 midColor;
uniform vec3 highColor;

// Texture Buffer Object for band energies
uniform samplerBuffer energyLevels; // This is the name we're binding to in C++

// Helper function to interpolate color (ported from C++)
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
    
    // Safe access
    int maxBars = int(numBars);
    if (barIndex >= maxBars) barIndex = maxBars - 1;
    
    // Fetch energy from TBO
    float energy = texelFetch(energyLevels, barIndex).r; 
    energy = clamp(energy, 0.0, 1.0); // Ensure energy is within [0, 1]

    // Use float for calculations
    float numBarsFloat = numBars;
    float barWidth = (aspectRatio * 2.0) / numBarsFloat;
    float spacing = barWidth * 0.05;
    float totalBarWidth = barWidth - spacing;
    float startX = -aspectRatio;

    float x = startX + float(barIndex) * barWidth;

    // Apply wave effect and zoom
    float wave = sin(time * 2.0 + float(barIndex) * 0.1) * 0.1 + 0.9;
    float height = energy * wave * zoomLevel * 2.0; // Scale height (0 to 2 range approx)
    height = max(height, 0.01); // Ensure a minimum height

    // Calculate color
    float hue = float(barIndex) / numBarsFloat;
    vec3 finalColor = interpolateColor(hue, energy, lowColor, midColor, highColor);
    Color = finalColor; // Pass color to fragment shader

    // --- Transform base quad vertex ---
    // Map aPos (0-1) to the bar's rectangle
    vec2 transformedPos = aPos;           // Start with 0-1 quad
    transformedPos.x *= totalBarWidth;    // Scale width
    transformedPos.y *= height;           // Scale height
    transformedPos.x += x;                // Shift to bar's x position
    transformedPos.y -= 1.0;              // Shift bottom to y = -1

    gl_Position = projection * view * vec4(transformedPos, 0.0, 1.0);
}
