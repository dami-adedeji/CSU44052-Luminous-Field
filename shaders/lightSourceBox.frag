#version 330 core

uniform vec3 lightColour;

out vec4 finalColour;

void main() {
    finalColour = vec4(lightColour, 1.0);
}