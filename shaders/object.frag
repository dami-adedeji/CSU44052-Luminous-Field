#version 330 core

in vec2 uv;
in vec3 colour;

out vec3 finalColour;

uniform sampler2D textureSampler;

void main() {
    finalColour = texture(textureSampler,uv).rgb; //for texture
    //finalColour = colour; //for colour in colour data array
}
