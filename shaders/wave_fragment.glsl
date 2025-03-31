#version 330 core

// Input from vertex shader
in vec3 fragColor;

// Output fragment color
out vec4 FragColor;

void main() {
    // Use color from vertex shader with full opacity
    FragColor = vec4(fragColor, 1.0);
} 