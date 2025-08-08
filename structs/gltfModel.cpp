#include "gltfModel.h"
#include "shader.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

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

            // === JOINTS_0 (bone IDs)
            std::vector<glm::uvec4> jointIndices(vertexCount, glm::uvec4(0));
            if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
                const auto& accessor = model.accessors[primitive.attributes.at("JOINTS_0")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                size_t stride = bufferView.byteStride > 0 ? bufferView.byteStride : 4 * (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? sizeof(uint16_t) : sizeof(uint8_t));
                const uint8_t* base = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

                for (size_t i = 0; i < vertexCount; ++i) {
                    const void* ptr = base + i * stride;

                    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                        const uint8_t* joints = reinterpret_cast<const uint8_t*>(ptr);
                        jointIndices[i] = glm::uvec4(joints[0], joints[1], joints[2], joints[3]);
                    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        const uint16_t* joints = reinterpret_cast<const uint16_t*>(ptr);
                        jointIndices[i] = glm::uvec4(joints[0], joints[1], joints[2], joints[3]);
                    }
                }
            }

            // === WEIGHTS_0 (bone weights)
            std::vector<glm::vec4> jointWeights(vertexCount, glm::vec4(0.0f));
            if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
                const auto& accessor = model.accessors[primitive.attributes.at("WEIGHTS_0")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                size_t stride = bufferView.byteStride > 0 ? bufferView.byteStride : 4 * sizeof(float);
                const uint8_t* base = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

                for (size_t i = 0; i < vertexCount; ++i) {
                    const float* weights = reinterpret_cast<const float*>(base + i * stride);
                    jointWeights[i] = glm::vec4(weights[0], weights[1], weights[2], weights[3]);

                    // Normalize
                    float sum = jointWeights[i].x + jointWeights[i].y + jointWeights[i].z + jointWeights[i].w;

                    if (sum > 0.0f)
                        jointWeights[i] /= sum;
                }
            }

            // === Combine into Vertex struct ===
            struct Vertex {
                glm::vec3 pos;
                glm::vec3 normal;
                glm::vec2 uv;
                glm::uvec4 jointIndices;
                glm::vec4 jointWeights;
            };

            std::vector<Vertex> vertices;
            for (size_t i = 0; i < vertexCount; ++i) {
                Vertex v;
                v.pos = glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);
                v.normal = normals ? glm::vec3(normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]) : glm::vec3(0.0f);
                v.uv = texcoords ? glm::vec2(texcoords[i * 2 + 0], texcoords[i * 2 + 1]) : glm::vec2(0.0f);
                v.jointIndices = jointIndices[i];
                v.jointWeights = jointWeights[i];
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

            glEnableVertexAttribArray(4); // bone IDs
            glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, jointIndices));

            glEnableVertexAttribArray(5); // bone weights
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, jointWeights));

            glBindVertexArray(0);

            // === Load texture if exists ===
            GLuint textureID = 0;
            glm::vec4 baseColorFactor = glm::vec4(1.0f);
            MeshPrimitive prim;
            if (primitive.material >= 0) {
                const auto& material = model.materials[primitive.material];
                const auto& pbr = material.pbrMetallicRoughness;

                if (!pbr.baseColorFactor.empty() && pbr.baseColorFactor.size() == 4) {
                    baseColorFactor = glm::vec4(
                        pbr.baseColorFactor[0],
                        pbr.baseColorFactor[1],
                        pbr.baseColorFactor[2],
                        pbr.baseColorFactor[3]);
                }

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

                    GLenum format = image.component == 3 ? GL_RGB : GL_RGBA;
                    glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.image.data());
                    glGenerateMipmap(GL_TEXTURE_2D);

                    hasTexture = true;
                }
            }

            prim.vao = vao;
            prim.indexCount = static_cast<GLuint>(indices.size());
            prim.textureID = textureID;
            prim.baseColorFactor = baseColorFactor;
            primitives.push_back(prim);
        }
    }

    // === Load skin (bones)
    for (const auto& skin : model.skins) {
        jointNodeIndices = skin.joints;

        // Map glTF node index to our bone index
        for (size_t i = 0; i < jointNodeIndices.size(); ++i) {
            nodeIndexToBone[jointNodeIndices[i]] = static_cast<int>(i);
        }

        const auto& accessor = model.accessors[skin.inverseBindMatrices];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        const float* matrixData = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

        for (size_t i = 0; i < accessor.count; ++i) {
            glm::mat4 inverseBind;
            memcpy(&inverseBind[0][0], &matrixData[i * 16], sizeof(glm::mat4));

            Bone bone;
            bone.inverseBindMatrix = inverseBind;
            bone.parentIndex = -1; // to be resolved
            bones.push_back(bone);
        }

        // Resolve bone hierarchy
        for (size_t i = 0; i < jointNodeIndices.size(); ++i) {
            int jointIndex = jointNodeIndices[i];
            for (size_t j = 0; j < jointNodeIndices.size(); ++j) {
                const auto& node = model.nodes[jointNodeIndices[j]];
                if (std::find(node.children.begin(), node.children.end(), jointIndex) != node.children.end()) {
                    bones[i].parentIndex = j;
                    break;
                }
            }
        }

        finalBoneMatrices.resize(bones.size(), glm::mat4(1.0f));
    }

    // === Load animations
    for (const auto& anim : model.animations) {
        Animation animation;

        for (const auto& sampler : anim.samplers) {
            AnimationSampler s;

            const auto& inputAccessor = model.accessors[sampler.input];
            const auto& inputBufferView = model.bufferViews[inputAccessor.bufferView];
            const auto& inputBuffer = model.buffers[inputBufferView.buffer];
            const float* inputData = reinterpret_cast<const float*>(&inputBuffer.data[inputBufferView.byteOffset + inputAccessor.byteOffset]);
            s.inputs.assign(inputData, inputData + inputAccessor.count);

            animation.maxTime = std::max(animation.maxTime, s.inputs.back());

            const auto& outputAccessor = model.accessors[sampler.output];
            const auto& outputBufferView = model.bufferViews[outputAccessor.bufferView];
            const auto& outputBuffer = model.buffers[outputBufferView.buffer];
            const float* outputData = reinterpret_cast<const float*>(&outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset]);

            if (outputAccessor.type == TINYGLTF_TYPE_VEC3) {
                for (size_t i = 0; i < outputAccessor.count; ++i) {
                    s.translations.emplace_back(outputData[i * 3 + 0], outputData[i * 3 + 1], outputData[i * 3 + 2]);
                }
            } else if (outputAccessor.type == TINYGLTF_TYPE_VEC4) {
                for (size_t i = 0; i < outputAccessor.count; ++i) {
                    s.rotations.emplace_back(outputData[i * 4 + 0], outputData[i * 4 + 1], outputData[i * 4 + 2], outputData[i * 4 + 3]);
                }
            }

            animation.samplers.push_back(s);
        }

        for (const auto& channel : anim.channels) {
            AnimationChannel c;
            c.targetNode = channel.target_node;
            c.path = channel.target_path;
            c.samplerIndex = channel.sampler;
            animation.channels.push_back(c);
        }

        animations.push_back(animation);
    }

    std::cout << "nodeIndexToBone mapping:\n";
    for (auto& [nodeIndex, boneIndex] : nodeIndexToBone) {
        std::cout << "Node " << nodeIndex << " -> Bone " << boneIndex << "\n";
    }

}

void GLTFModel::updateAnimation(float deltaTime)
{
    if (!isAnimated) return;

    animationTime += deltaTime;
    const Animation& anim = animations[0];

    // Loop animation
    if (animationTime > anim.maxTime)
        animationTime = fmod(animationTime, anim.maxTime);

    // Per-joint transform components
    std::vector<glm::vec3> translations(bones.size(), glm::vec3(0.0f));
    std::vector<glm::quat> rotations(bones.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> scales(bones.size(), glm::vec3(1.0f));
    std::vector<bool> hasTranslation(bones.size(), false);
    std::vector<bool> hasRotation(bones.size(), false);
    std::vector<bool> hasScale(bones.size(), false);

    for (const auto& channel : anim.channels) {
        const auto& sampler = anim.samplers[channel.samplerIndex];
        if (sampler.inputs.empty()) continue;

        size_t prev = 0;
        while (prev < sampler.inputs.size() - 1 && animationTime > sampler.inputs[prev + 1]) {
            ++prev;
        }
        size_t next = std::min(prev + 1, sampler.inputs.size() - 1);

        float t1 = sampler.inputs[prev];
        float t2 = sampler.inputs[next];
        float factor = (animationTime - t1) / (t2 - t1 + 1e-6f);

        // Use nodeIndexToBone map
        auto it = nodeIndexToBone.find(channel.targetNode);
        if (it == nodeIndexToBone.end()) continue;
        int boneIndex = it->second;

        if (channel.path == "translation" && !sampler.translations.empty()) {
            glm::vec3 interp = glm::mix(sampler.translations[prev], sampler.translations[next], factor);
            translations[boneIndex] = interp;
            hasTranslation[boneIndex] = true;
        }
        else if (channel.path == "rotation" && !sampler.rotations.empty()) {
            glm::vec4 rot1 = sampler.rotations[prev];
            glm::vec4 rot2 = sampler.rotations[next];
            glm::quat q1 = glm::quat(rot1.w, rot1.x, rot1.y, rot1.z);
            glm::quat q2 = glm::quat(rot2.w, rot2.x, rot2.y, rot2.z);
            rotations[boneIndex] = glm::slerp(q1, q2, factor);
            hasRotation[boneIndex] = true;
        }
        else if (channel.path == "scale" && !sampler.scales.empty()) {
            glm::vec3 interp = glm::mix(sampler.scales[prev], sampler.scales[next], factor);
            scales[boneIndex] = interp;
            hasScale[boneIndex] = true;
        }
    }

    // Build local transforms
    for (size_t i = 0; i < bones.size(); ++i) {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), hasTranslation[i] ? translations[i] : glm::vec3(0.0f));
        glm::mat4 R = glm::mat4_cast(hasRotation[i] ? rotations[i] : glm::quat(1, 0, 0, 0));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), hasScale[i] ? scales[i] : glm::vec3(1.0f));
        bones[i].localTransform = T * R * S;
    }

    // Global transforms
    for (size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].parentIndex < 0)
            bones[i].globalTransform = bones[i].localTransform;
        else
            bones[i].globalTransform = bones[bones[i].parentIndex].globalTransform * bones[i].localTransform;
    }

    // Final bone matrices
    for (size_t i = 0; i < bones.size(); ++i) {
        finalBoneMatrices[i] = bones[i].globalTransform * bones[i].inverseBindMatrix;
    }
}



void GLTFModel::render(Shader& program, GLuint shadowMap)
{
    program.setFloat("diffuseStrength", diffuseStrength);
    program.setMatrix("model", &modelMatrix[0][0]);
    program.setBool("useSkinning", isAnimated);

    program.setMatrixArray("bones", finalBoneMatrices);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    program.setInt("shadowMap", 1);

    for (const auto& prim : primitives)
    {
        program.setBool("useTexture", hasTexture);
        if (hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, prim.textureID);
            program.setInt("textureSampler", 0); // shader uniform
        }
        else program.setVec4("modelColour", prim.baseColorFactor);

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
