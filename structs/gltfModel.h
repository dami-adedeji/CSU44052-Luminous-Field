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
    GLTFModel(const std::string& path);

    void render(Shader& shader, GLuint shadowMap);

    void setTransform(const glm::mat4& transform);

    void renderDepth(glm::mat4& lightSpaceMatrix, Shader& shader);

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
};

#endif