#version 330 core

uniform vec3 spotlightColour;

out vec4 finalColour;

void main() {
    finalColour = vec4(spotlightColour, 1.0);
}