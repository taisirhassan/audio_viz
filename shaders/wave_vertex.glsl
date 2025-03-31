#version 330 core
layout (location = 0) in float aX; // Base line segment vertex x-position (0 or 1)

// Outputs
out vec3 Color;

// Uniforms
uniform mat4 view;
uniform mat4 projection;
uniform float screenWidth; // For calculating horizontal positions
uniform float aspectRatio;
uniform float zoomLevel;
uniform float time;
uniform vec3 lowColor;
uniform vec3 midColor;
uniform vec3 highColor;
uniform int audioDataSize;

// Texture Buffer Object for raw audio data
uniform samplerBuffer audioDataSampler;

// Helper function to interpolate color (same as others)
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

// Helper function to get interpolated audio sample
float getAudioSample(float t) {
    if (audioDataSize == 0) return 0.0;
    int index = int(t);
    float frac = fract(t);
    
    // Clamp index to valid range
    index = clamp(index, 0, audioDataSize - 1);
    int nextIndex = min(index + 1, audioDataSize - 1);
    
    float sample1 = texelFetch(audioDataSampler, index).r;
    float sample2 = texelFetch(audioDataSampler, nextIndex).r;
    
    return mix(sample1, sample2, frac);
}

void main() {
    // --- Calculate segment properties based on instance ID ---
    // We draw screenWidth-1 segments. InstanceID goes from 0 to screenWidth-2.
    int segmentIndex = gl_InstanceID; 
    
    // Calculate x position in normalized device coordinates (-aspect to +aspect)
    float x_norm_start = (float(segmentIndex) / (screenWidth - 1.0)) * (aspectRatio * 2.0) - aspectRatio;
    float x_norm_end = (float(segmentIndex + 1) / (screenWidth - 1.0)) * (aspectRatio * 2.0) - aspectRatio;
    
    // Calculate corresponding time 't' in audio samples
    float step = (audioDataSize > 0) ? float(audioDataSize) / screenWidth : 0.0;
    float t_start = float(segmentIndex) * step;
    float t_end = float(segmentIndex + 1) * step;

    // Get audio samples
    float y_sample_start = getAudioSample(t_start);
    float y_sample_end = getAudioSample(t_end);

    // Apply effects (scaling, wave)
    float amplitudeScale = zoomLevel * 0.8 * 5.0; // Combined scale from C++
    float wave_start = 1.0 + sin(time * 1.5 + x_norm_start * 0.01) * 0.08; // Note: C++ used x, might need adjustment
    float wave_end = 1.0 + sin(time * 1.5 + x_norm_end * 0.01) * 0.08;

    float y_start = y_sample_start * amplitudeScale * wave_start;
    float y_end = y_sample_end * amplitudeScale * wave_end;

    // Clamp y values
    y_start = clamp(y_start, -1.0, 1.0);
    y_end = clamp(y_end, -1.0, 1.0);

    // Determine current vertex position based on aX (0 or 1)
    float currentX, currentY;
    if (aX < 0.5) { // Start of the segment (aX=0)
        currentX = x_norm_start;
        currentY = y_start;
    } else { // End of the segment (aX=1)
        currentX = x_norm_end;
        currentY = y_end;
    }
    
    // Calculate color (based on start of segment for simplicity)
    float progress = float(segmentIndex) / (screenWidth - 1.0);
    float energy = abs(y_start);
    Color = interpolateColor(progress, energy, lowColor, midColor, highColor);

    gl_Position = projection * view * vec4(currentX, currentY, 0.0, 1.0);
} 