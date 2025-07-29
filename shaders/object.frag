#version 330 core

in vec2 uv;
in vec3 normal;
in vec3 colour;
in vec3 worldPos;
out vec4 finalColour;

uniform sampler2D textureSampler;
uniform vec3 lightColour;
uniform sampler2D normalSampler;
uniform vec3 lightPos;

// for a spotlight
uniform float cutoff;
uniform vec3 lightFacing;

// attenuation
uniform float constant;
uniform float linear;
uniform float quadratic;

void main() {
    vec3 baseColour = texture(textureSampler,uv).rgb; //for texture

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - worldPos);

    float theta = dot(lightDir, normalize(-lightFacing));

    float intensity = (theta > cutoff) ? 1.0f : 0.3f;

    float ambientStrength = 0.3f;
    vec3 ambient = ambientStrength * lightColour;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColour;

    // attenuation
    float dist = length(lightPos - worldPos);
    float attenuation = 1.0 / (constant + linear * dist + quadratic * (dist * dist));

    // apply lighting
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;

    vec3 result = (ambient + diffuse) * baseColour;
    finalColour = vec4(result, 1.0);

    /*tone mapping
    result = result / 1.0 + result;

    // gamma correction
    float gamma = 2.2;
    result = pow(result, vec3(1.0/gamma));
    /*float exposure = 0.5f; // because my scene is too bright and idk how else to reduce that
    finalColor = finalColor * exposure;*/
}
