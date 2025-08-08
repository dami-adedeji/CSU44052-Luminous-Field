#version 330 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec3 vertexNorm;
//layout (location = 2) in vec3 vertexCol;
layout (location = 3) in vec2 vertexUV;
layout (location = 4) in ivec4 jointIndices;
layout (location = 5) in vec4 jointWeights;
layout (location = 6) in mat4 instanceMatrix;

uniform mat4 bones[100];

//out vec3 surfaceColour;
out vec3 normal;
out vec2 uv;
out vec3 fragPos;
out vec4 fragPosLightSpace;

uniform mat4 instanceMatrix;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform bool useSkinning;

void main() {
    vec4 skinnedPos;
    vec3 skinnedNorm = vertexNorm;
    if (useSkinning) {
        mat4 skinMatrix =
        jointWeights.x * bones[int(jointIndices.x)] +
        jointWeights.y * bones[int(jointIndices.y)] +
        jointWeights.z * bones[int(jointIndices.z)] +
        jointWeights.w * bones[int(jointIndices.w)];
        skinnedPos = skinMatrix * vec4(vertexPos, 1.0);
        skinnedNorm = normalize(mat3(skinMatrix) * vertexNorm);
    } else {
        skinnedPos = vec4(vertexPos, 1.0);
    }

    vec4 worldPos = instanceMatrix * skinnedPos;
    gl_Position = projection * view * worldPos;

    // surfaceColour = vertexCol;
    uv = vertexUV;
    normal = mat3(transpose(inverse(instanceMatrix))) * skinnedNorm;
    fragPos = vec3(worldPos);
    fragPosLightSpace = lightSpaceMatrix * worldPos;
}
