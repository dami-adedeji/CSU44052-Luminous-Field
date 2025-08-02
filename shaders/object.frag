#version 330 core

#define MAX_LIGHTS 8
struct Light
{
    int type;
    vec3 position;
    vec3 direction;
    vec3 colour;
    float constant;
    float linear;
    float quadratic;
    float cutoff;
    float outerCutoff;
};

in vec2 uv;
in vec3 normal;
in vec3 surfaceColour;
in vec3 fragPos;

out vec4 finalColour;

uniform int numLights;
uniform Light lights[MAX_LIGHTS];

uniform bool useTexture;
uniform sampler2D textureSampler;

uniform vec3 cameraPos;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 fogColour;

void main() {
    vec3 baseColour;
    if (useTexture) baseColour = texture(textureSampler, uv).rgb;
    else baseColour = surfaceColour;

    vec3 result = vec3(0.0);
    for (int i = 0; i < numLights; ++i) {
        Light light = lights[i];
        vec3 lightDir;
        float attenuation = 1.0;
        float diff = 0.0;

        if (light.type == 0) {
            // Directional
            lightDir = normalize(-light.direction);
            diff = max(dot(normal, lightDir), 0.0);

        } else if (light.type == 1) {
            // Point
            lightDir = normalize(light.position - fragPos);
            float distance = length(light.position - fragPos);
            attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
            diff = max(dot(normal, lightDir), 0.0);

        } else if (light.type == 2) {
            // Spotlight
            lightDir = normalize(light.position - fragPos);
            float theta = dot(lightDir, normalize(-light.direction));
            float epsilon = light.cutoff - light.outerCutoff;
            float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

            float distance = length(light.position - fragPos);
            attenuation = intensity / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
            diff = max(dot(normal, lightDir), 0.0);
        }

        vec3 lighting = diff * light.colour * baseColour;
        result += attenuation * lighting;
    }
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * baseColour;
    result += ambient;

    float distanceToCamera = length(fragPos - cameraPos);
    float fogFactor = clamp((fogEnd - distanceToCamera) / (fogEnd - fogStart), 0.0, 1.0);
    // Blend scene color with fog
    vec3 foggedColour = mix(fogColour, result, fogFactor);
    finalColour = vec4(foggedColour, 1.0);
}

