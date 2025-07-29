#ifndef _TILE_H_
#define _TILE_H_

#include <glad/gl.h>
#include "shader.h"
#include <glm/glm.hpp>

struct Tile
{
    glm::vec3 position;
    static constexpr float tileSize = 250.0f;

    static const GLfloat vertices[12];
    static const GLfloat colours[12];
    static const GLfloat uv[8];
    static const GLint indices[6];

    GLuint VAO, VBO, UVBO, EBO, CBO, textureID;

    void initialise(glm::vec3 position, Shader program, const char* texture_path);

    void render(glm::mat4 cameraMatrix, Shader &program);

    void cleanup();
};

#endif