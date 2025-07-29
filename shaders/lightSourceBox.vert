#version 330 core

layout (location = 0) in vec3 vertexPos;

out vec3 colour;
uniform mat4 MVP;
uniform vec3 lightColour;
void main() {
    gl_Position = MVP * vec4(vertexPos, 1.0);
    //uv = vertexUV;
    //normal = vec3(0.0f, 1.0f, 0.0f);
}