#version 330 core

// Input from vertex shader
in vec3 fragColor;

// Output fragment color
out vec4 FragColor;

void main() {
    // Add a subtle glow effect by using the alpha channel
    float alpha = 0.95; // Slightly transparent for glow effect
    
    // Add slight color boost and saturation
    vec3 finalColor = fragColor * 1.1; // Boost colors
    
    // Output final color with alpha
    FragColor = vec4(finalColor, alpha);
} 