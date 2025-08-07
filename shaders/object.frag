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
//in vec3 surfaceColour;
in vec3 fragPos;
in vec4 fragPosLightSpace;

out vec4 finalColour;

uniform int numLights;
uniform Light lights[MAX_LIGHTS];

uniform bool useTexture;
uniform sampler2D textureSampler;
uniform sampler2D shadowMap;

uniform vec3 cameraPos;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 fogColour;

float calculateShadow()
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // perspective divide

    projCoords = projCoords * 0.5 + 0.5; // ndc

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0 ||
    projCoords.z > 1.0)
    return 1.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;

    float currentDepth = projCoords.z;

    float shadow = (currentDepth >= (closestDepth + 1e-3)) ? 0.2 : 1.0;

    return shadow;
}

void main() {
    vec3 baseColour;
    if (useTexture) baseColour = texture(textureSampler, uv).rgb;
    //else baseColour = surfaceColour;

    vec3 result = vec3(0.0);
    for (int i = 0; i < numLights; ++i) {
        Light light = lights[i];
        vec3 lightDir;
        float attenuation = 1.0;
        float diff = 0.0;
        float shadow = 1.0;
        if (light.type == 0) {
            // Directional
            lightDir = normalize(-light.direction);
            diff = max(dot(normal, lightDir), 0.0);
            shadow = calculateShadow();
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

        vec3 lighting = diff * light.colour * baseColour * shadow;
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

