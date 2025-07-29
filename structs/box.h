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

    static const GLfloat vertex_buffer_data[72];
    static const GLfloat color_buffer_data[72];
    static const GLuint index_buffer_data[36];

    // OpenGL buffers
    GLuint vertexArrayID, vertexBufferID, indexBufferID, colorBufferID;

    void initialize(glm::vec3 scale, glm::vec3 position, Shader &program);

    void render(glm::mat4 cameraMatrix, Shader &program);

    void cleanup();
};

#endif