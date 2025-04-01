#version 330 core

// Input vertex data 
layout(location = 0) in vec2 position;

// Instance ID will be used to position each line segment
uniform int numSamples;
uniform float waveScale;
uniform float time;

// Texture buffer for audio data
uniform samplerBuffer audioSampler;
uniform samplerBuffer audioDataSampler; // Fallback

// Colors for wave gradient
uniform vec3 lowColor;
uniform vec3 midColor; 
uniform vec3 highColor;

// MVP matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Outputs to fragment shader
out vec3 fragColor;

// Helper function to interpolate colors based on position and energy
vec3 interpolateColor(float position, float energy) {
    // Normalize energy for coloring (clamp to avoid extreme values)
    float normEnergy = clamp(abs(energy), 0.0, 1.0);
    
    // Use both position (x coordinate) and energy for coloring
    // Position determines basic color choice (low to high freq across x-axis)
    // Energy affects brightness/intensity
    
    // Position-based color (low freq left to high freq right)
    vec3 baseColor;
    if (position < 0.33) {
        // Left side - use low color to mid color
        float t = position / 0.33;
        baseColor = mix(lowColor, midColor, t);
    } else if (position < 0.66) {
        // Middle - use mid color to high color
        float t = (position - 0.33) / 0.33;
        baseColor = mix(midColor, highColor, t);
    } else {
        // Right side - use high color back to low color (wrap around)
        float t = (position - 0.66) / 0.34;
        baseColor = mix(highColor, lowColor, t);
    }
    
    // Enhanced energy affects brightness/saturation
    float brightness = 0.7 + normEnergy * 0.6; // Increased base brightness and range
    
    return baseColor * brightness;
}

// Helper function to get audio sample, preferring audioSampler
float getAudioSample(int index) {
    float sample = 0.0;
    if (index >= 0 && index < numSamples) {
        // Prioritize audioSampler
        // Assuming C++ side binds the TBO to texture unit 0 and sets one of these uniforms
        sample = texelFetch(audioSampler, index).r;
        // Fallback could be added here if needed, e.g.:
        // if (!texture(audioSampler, vec2(0)).r) { // Not a reliable check!
        //     sample = texelFetch(audioDataSampler, index).r;
        // }
    }
    return sample;
}

void main() {
    // Get instance ID and calculate segment position
    int segmentIndex = gl_InstanceID;
    float totalSegments = float(numSamples);
    
    // Skip if out of range
    if (segmentIndex >= numSamples) {
        gl_Position = vec4(0.0);
        return;
    }
    
    // Calculate the horizontal position for this segment
    float xPosition = (float(segmentIndex) / (totalSegments - 1.0)) * 2.0 - 1.0;
    
    // Normalized position (0 to 1) for color interpolation
    float normalizedPosition = float(segmentIndex) / (totalSegments - 1.0);
    
    // Adjust segment for width and spacing
    float segmentWidth = 2.0 / totalSegments;
    
    // Use start or end point of line depending on position.x (0 or 1)
    float xOffset = 0.0;
    if (position.x > 0.5) {
        // This is the end point of the line
        xOffset = segmentWidth;
    }
    
    // Get audio sample value using the helper
    float audioValue = getAudioSample(segmentIndex);
    
    // Apply enhanced scaling for better visualization
    audioValue *= waveScale * 3.0; // Increased scale factor
    
    // Add more dynamic animation
    float timeScale = 2.0;
    float waveFreq = 15.0;
    float animAmplitude = 0.1;
    
    // Primary wave animation
    audioValue += animAmplitude * sin(waveFreq * xPosition + time * timeScale);
    
    // Secondary wave for more complexity
    audioValue += animAmplitude * 0.5 * cos(waveFreq * 0.5 * xPosition - time * timeScale * 0.7);
    
    // Add subtle vertical offset based on position to create wave-like effect
    audioValue += 0.15 * sin(normalizedPosition * 3.14159 * 2.0 + time);
    
    // Add frequency-based modulation
    float freqMod = sin(time * 0.5) * 0.5 + 0.5; // 0 to 1 oscillation
    audioValue *= 1.0 + freqMod * 0.2; // Up to 20% additional amplitude
    
    // Set position with audio value for height
    vec4 vertexPosition = vec4(xPosition + xOffset, audioValue, 0.0, 1.0);
    
    // Calculate final position using MVP matrices
    gl_Position = projection * view * model * vertexPosition;
    
    // Enhanced color interpolation based on audio value and position
    float energyFactor = abs(audioValue) * 2.0; // Increase color intensity
    vec3 baseColor = interpolateColor(normalizedPosition, energyFactor);
    
    // Add time-based color modulation
    float colorMod = sin(time * 1.5 + normalizedPosition * 6.28318) * 0.2 + 1.0;
    fragColor = baseColor * colorMod;
} 