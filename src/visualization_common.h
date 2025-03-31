#pragma once

// Array sizes
constexpr int QUAD_VERTICES_SIZE = 12; // 6 vertices * 2 components
constexpr int CUBE_VERTICES_SIZE = 108; // 36 vertices * 3 components
constexpr int CUBE_NORMALS_SIZE = 108; // 36 normals * 3 components

// Common vertex data used by multiple visualization styles
extern const float quadVertices[QUAD_VERTICES_SIZE];
extern const float cubeVertices[CUBE_VERTICES_SIZE];
extern const float cubeNormals[CUBE_NORMALS_SIZE]; 