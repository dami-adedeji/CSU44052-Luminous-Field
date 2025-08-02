#version 330 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 vertexNorm;
layout (location = 2) in vec3 vertexCol;
layout (location = 3) in vec2 vertexUV;

out vec3 surfaceColour;
out vec3 normal;
out vec2 uv;
out vec3 fragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(vertexPos, 1.0);

    surfaceColour = vertexCol;
    uv = vertexUV;
    normal = vertexNorm;
    fragPos = vec3(model * vec4(vertexPos, 1.0));
}
