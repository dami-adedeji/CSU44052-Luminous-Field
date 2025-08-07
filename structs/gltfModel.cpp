#include "gltfModel.h"
#include "shader.h"
#include <iostream>

GLTFModel::GLTFModel(const std::string& path)
{
    loadModel(path);
}

void GLTFModel::loadModel(const std::string& path)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    bool ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!ok) {
        std::cerr << "Failed to load glTF: " << err << std::endl;
        return;
    } else {
        std::cout << "Successfully loaded glTF." << std::endl;
    }

    // For each mesh
    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            // === Extract attributes ===
            const auto& positionAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
            const auto& positionBufferView = model.bufferViews[positionAccessor.bufferView];
            const auto& positionBuffer = model.buffers[positionBufferView.buffer];

            const float* positions = reinterpret_cast<const float*>(
                &positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset]);
            size_t vertexCount = positionAccessor.count;

            const float* normals = nullptr;
            if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                const auto& normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                const auto& normalBufferView = model.bufferViews[normalAccessor.bufferView];
                const auto& normalBuffer = model.buffers[normalBufferView.buffer];
                normals = reinterpret_cast<const float*>(
                    &normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]);
            }

            const float* texcoords = nullptr;
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                const auto& texAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                const auto& texBufferView = model.bufferViews[texAccessor.bufferView];
                const auto& texBuffer = model.buffers[texBufferView.buffer];
                texcoords = reinterpret_cast<const float*>(
                    &texBuffer.data[texBufferView.byteOffset + texAccessor.byteOffset]);
            }

            // === Combine into Vertex struct ===
            struct Vertex {
                glm::vec3 pos;
                glm::vec3 normal;
                glm::vec2 uv;
            };

            std::vector<Vertex> vertices;
            for (size_t i = 0; i < vertexCount; ++i) {
                Vertex v;
                v.pos = glm::vec3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
                v.normal = normals ? glm::vec3(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]) : glm::vec3(0.0f);
                v.uv = texcoords ? glm::vec2(texcoords[i * 2], texcoords[i * 2 + 1]) : glm::vec2(0.0f);
                vertices.push_back(v);
            }

            // === Load Indices ===
            const auto& indexAccessor = model.accessors[primitive.indices];
            const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
            const auto& indexBuffer = model.buffers[indexBufferView.buffer];

            std::vector<unsigned int> indices;
            const void* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

            for (size_t i = 0; i < indexAccessor.count; ++i) {
                if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    indices.push_back(((const uint16_t*)indexData)[i]);
                } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    indices.push_back(((const uint32_t*)indexData)[i]);
                } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    indices.push_back(((const uint8_t*)indexData)[i]);
                }
            }

            // === Upload to OpenGL ===
            GLuint vao, vbo, ebo;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // Vertex attributes
            glEnableVertexAttribArray(0); // position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

            glEnableVertexAttribArray(1); // normal
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

            glEnableVertexAttribArray(3); // texcoords
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

            glBindVertexArray(0);

            // === Load texture if exists ===
            GLuint textureID = 0;
            glm::vec4 baseColorFactor = glm::vec4(1.0f);

            if (primitive.material >= 0) {
                const auto& material = model.materials[primitive.material];
                const auto& pbr = material.pbrMetallicRoughness;

                // Get baseColorFactor if it exists
                if (!pbr.baseColorFactor.empty() && pbr.baseColorFactor.size() == 4) {
                    baseColorFactor = glm::vec4(
                        pbr.baseColorFactor[0],
                        pbr.baseColorFactor[1],
                        pbr.baseColorFactor[2],
                        pbr.baseColorFactor[3]
                    );
                }

                // Load texture if it exists
                if (pbr.baseColorTexture.index >= 0) {
                    int texIndex = pbr.baseColorTexture.index;
                    const auto& texture = model.textures[texIndex];
                    const auto& image = model.images[texture.source];

                    glGenTextures(1, &textureID);
                    glBindTexture(GL_TEXTURE_2D, textureID);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    GLenum format = GL_RGBA;
                    if (image.component == 3)
                        format = GL_RGB;

                    glTexImage2D(GL_TEXTURE_2D, 0, format,
                        image.width, image.height, 0, format,
                        GL_UNSIGNED_BYTE, image.image.data());
                    glGenerateMipmap(GL_TEXTURE_2D);

                    hasTexture = true;
                }
            }

            // === Save primitive with textureID ===
            primitives.push_back({ vao, (GLuint)indices.size(), textureID });
        }
    }
}


void GLTFModel::render(Shader& program, GLuint shadowMap )
{
    program.setMatrix("model", &modelMatrix[0][0]);

    for (const auto& prim : primitives)
    {
        program.setBool("useTexture", hasTexture);
        if (hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, prim.textureID);
            program.setInt("textureSampler", 0); // shader uniform
        }
        else program.setVec4("modelColour", prim.baseColorFactor);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        program.setInt("shadowMap", 1);

        glBindVertexArray(prim.vao);
        glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}

void GLTFModel::renderDepth(glm::mat4& lightSpaceMatrix, Shader& program)
{
    program.setMatrix("model",&modelMatrix[0][0]);
    program.setMatrix("lightSpaceMatrix", &lightSpaceMatrix[0][0]);

    for (const auto& prim : primitives)
    {
        // textures unimportant for depth
        glBindVertexArray(prim.vao);
        glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}
void GLTFModel::setTransform(const glm::mat4& transform) {
    modelMatrix = transform;
}
