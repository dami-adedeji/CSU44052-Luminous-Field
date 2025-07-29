#version 330 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 vertexCol;
layout (location = 2) in vec2 vertexUV;

out vec3 colour;
out vec3 normal;
out vec2 uv;
out vec3 worldPos;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(vertexPos, 1.0);
    colour = vertexCol;
    uv = vertexUV;
    normal = vec3(0.0f, 1.0f, 0.0f);
    worldPos = vertexPos;
}
