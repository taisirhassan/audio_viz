#version 330 core
layout (location = 0) in vec3 aPos;   // Base cube vertex position (-0.5 to 0.5 range)

// Outputs
out vec3 Color;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float aspectRatio;
uniform float numBars;
uniform float zoomLevel;
uniform float time;
uniform float rotationSpeed;
uniform vec3 lowColor;
uniform vec3 midColor;
uniform vec3 highColor;

// Texture Buffer Object for band energies
uniform samplerBuffer bandEnergiesSampler;

// Helper function to interpolate color
vec3 interpolateColor(float position, float energy) {
    vec3 color;
    float hueNormalized = mod(position + energy * 0.2, 1.0);
    if (hueNormalized < 0.33) {
        color = mix(lowColor, midColor, hueNormalized / 0.33);
    } else if (hueNormalized < 0.66) {
        color = mix(midColor, highColor, (hueNormalized - 0.33) / 0.33);
    } else {
        color = mix(highColor, lowColor, (hueNormalized - 0.66) / 0.33);
    }
    float brightness = 0.5 + energy * 0.5;
    return color * brightness;
}

// Helper function to safely fetch energy
float getEnergy(int barIndex) {
    // Clamp index to prevent out-of-bounds access
    int clampedIndex = clamp(barIndex, 0, int(numBars) - 1);
    if (barIndex >= int(numBars) || barIndex < 0) {
        // Optional: Indicate out-of-bounds fetch visually or return zero
        // return 0.0; // Return zero if out of bounds
    }
    return texelFetch(bandEnergiesSampler, clampedIndex).r;
}

void main() {
    // --- Calculate Bar Properties ---
    int barIndex = gl_InstanceID;
    float energy = getEnergy(barIndex); 
    float numBarsFloat = float(max(1.0, numBars));

    float barWidth = 0.8 / numBarsFloat;
    float spacing = 0.2 / numBarsFloat;
    float xCenter = (float(barIndex) / numBarsFloat) - 0.5 + (0.5 / numBarsFloat);
    
    // --- Calculate Vertex Position --- 
    // Base vertex position from VBO (assuming aPos is a unit cube from -0.5 to 0.5)
    vec3 pos = aPos; 

    // Calculate final scaled height
    float scaledHeight = max(0.01, energy); // Minimum height of 0.01

    // Scale X and Z dimensions based on barWidth
    pos.x *= barWidth * 0.5; // Scale from center
    pos.z *= barWidth * 0.5; // Scale from center

    // Scale Y: Map original [-0.5, 0.5] range to [0, scaledHeight]
    pos.y = (pos.y + 0.5) * scaledHeight;

    // Translate bar to its correct x position
    pos.x += xCenter;
    
    // Z remains centered relative to the bar's own width/depth

    // Transform final position
    gl_Position = projection * view * model * vec4(pos, 1.0);

    // --- Calculate Color --- 
    float hue = float(barIndex) / numBarsFloat;
    vec3 finalColor = interpolateColor(hue, energy);

    // **** DIAGNOSTIC: Make bar MAGENTA if energy is very low ****
    if (energy < 0.01) { 
        finalColor = vec3(1.0, 0.0, 1.0); // Magenta if flat
    }
    // **** END DIAGNOSTIC ****

    Color = finalColor;
} 