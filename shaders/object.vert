#version 330 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 vertexCol;
layout (location = 2) in vec2 vertexUV;

out vec3 colour;
out vec2 uv;
uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(vertexPos, 1.0);
    colour = vertexCol;
    uv = vertexUV;
}
