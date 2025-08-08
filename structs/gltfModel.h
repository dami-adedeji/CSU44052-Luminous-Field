#ifndef _GLTF_MODEL_H_
#define _GLTF_MODEL_H_

#include <string>
#include <vector>
#include <glad/gl.h>  // or GLEW/GL3W
#include <glm/glm.hpp>
#include <shader.h>
#include <tiny_gltf.h>

struct GLTFModel
{
public:
    float diffuseStrength = 1.0f;

    bool isAnimated;

    GLTFModel(const std::string& path);

    void render(Shader& shader, GLuint shadowMap);

    void setTransform(const glm::mat4& transform);

    void renderDepth(glm::mat4& lightSpaceMatrix, Shader& shader);

    void updateAnimation(float deltaTime);

private:
    void loadModel(const std::string& path);

    struct MeshPrimitive {
        GLuint vao;
        GLuint indexCount;
        GLuint textureID = 0;
        glm::vec4 baseColorFactor;
        bool hasTexture;
    };

    tinygltf::Model model;
    bool hasTexture = true;
    std::vector<MeshPrimitive> primitives;
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    struct Bone {
        int parentIndex;
        glm::mat4 inverseBindMatrix;
        glm::mat4 localTransform;
        glm::mat4 globalTransform;
    };

    struct AnimationSampler {
        std::vector<float> inputs;              // keyframe times
        std::vector<glm::vec4> rotations;       // quaternion
        std::vector<glm::vec3> translations;
        std::vector<glm::vec3> scales;
    };

    struct AnimationChannel {
        int targetNode;
        std::string path; // "rotation", "translation", "scale"
        int samplerIndex;
    };

    struct Animation {
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
        float maxTime = 0.0f;
    };

    std::vector<Bone> bones;
    std::vector<int> jointNodeIndices; // glTF node indices
    std::vector<Animation> animations;
    float animationTime = 0.0f;
    std::unordered_map<int, int> nodeIndexToBone; // maps glTF node index → our bone index
    std::vector<glm::mat4> finalBoneMatrices;

};

#endif