#version 330 core

layout (location = 0) in vec3 vertexPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(vertexPos, 1.0);
    //uv = vertexUV;
    //normal = vec3(0.0f, 1.0f, 0.0f);
}