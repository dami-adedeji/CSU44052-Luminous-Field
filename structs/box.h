#ifndef _BOX_H_
#define _BOX_H_

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include "shader.h"
struct Box
{
    glm::vec3 position;			// Position of the box
    glm::vec3 scale;			// Size of the box in each axis
    const char* texture_path;

    static const GLfloat vertex_buffer_data[72];
    static const GLfloat normal_buffer_data[72];
    static const GLfloat color_buffer_data[72];
    static const GLuint index_buffer_data[36];

    bool hasTexture;

    // OpenGL buffers
    GLuint vertexArrayID, vertexBufferID, normalBufferID, indexBufferID, colorBufferID, textureID;

    void initialize(glm::vec3 scale, glm::vec3 position, Shader &program, const char *texture_path);

    void render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Shader &program);

    void renderDepth(Shader &program);

    void cleanup();
};

#endif