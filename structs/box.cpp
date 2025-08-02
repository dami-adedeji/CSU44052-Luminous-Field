#include "box.h"
#include "shader.h"
#include "texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

const GLfloat Box::vertex_buffer_data[72] = {	// Vertex definition for a canonical box
    // Front face
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,

    // Back face
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,

    // Left face
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,

    // Right face
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,

    // Top face
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,

    // Bottom face
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
};

const GLfloat Box::normal_buffer_data[72] = {
    // Front face (0, 0, 1)
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,

   // Back face (0, 0, -1)
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,

   // Left face (-1, 0, 0)
   -1.0f, 0.0f, 0.0f,
   -1.0f, 0.0f, 0.0f,
   -1.0f, 0.0f, 0.0f,
   -1.0f, 0.0f, 0.0f,

   // Right face (1, 0, 0)
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,

   // Top face (0, 1, 0)
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,

   // Bottom face (0, -1, 0)
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f
};


const GLfloat Box::color_buffer_data[72] = {
    // Front, red
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,

    // Back, yellow
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,

    // Left, green
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,

    // Right, cyan
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,

    // Top, blue
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,

    // Bottom, magenta
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
};

const GLuint Box::index_buffer_data[36] = {		// 12 triangle faces of a box
    0, 1, 2,
    0, 2, 3,

    4, 5, 6,
    4, 6, 7,

    8, 9, 10,
    8, 10, 11,

    12, 13, 14,
    12, 14, 15,

    16, 17, 18,
    16, 18, 19,

    20, 21, 22,
    20, 22, 23
};

void Box::initialize(glm::vec3 scale, glm::vec3 position, Shader &program, const char *texture_path) {
    // Define scale of the box geometry;
    this->scale = scale;
    this->position = position + glm::vec3(0.0f, scale.y, 0.0f);
    //this->rotateAxis = rotateAxis;
    program.use();

    // Create a vertex array object
    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    // Create a vertex buffer object to store the vertex data
    glGenBuffers(1, &vertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

    // Create a vertex buffer object to store the normal data
    glGenBuffers(1, &normalBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);

    if (texture_path == nullptr || texture_path[0] == '\0')
    {
        hasTexture = false;
        // Create a vertex buffer object to store the color data
        glGenBuffers(1, &colorBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);
    }
    else
    {
        hasTexture = true;
        textureID = LoadTextureTileBox(texture_path);
        if (!glIsTexture(textureID))
            std::cerr << "Invalid texture loaded from " << texture_path << std::endl;
    }

    // Create an index buffer object to store the index data that defines triangle faces
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

    // Get a handle for our "MVP" uniform
    //mvpMatrixID = glGetUniformLocation(shaderID, "MVP");
}

void Box::render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Shader &program) {
    program.use();

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    if (hasTexture == false)
    {
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        // set texture sampler to use texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        //textureSamplerID = glGetUniformLocation(shaderID, "textureSampler");
        //glUniform1i(textureSamplerID, 0);
        program.setInt("textureSampler", 0);
    }
    program.setBool("useTexture", hasTexture);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

    // TODO: Model transform
    // ------------------------------------
    glm::mat4 modelMatrix = glm::mat4();
    // Translate the box to its position
    modelMatrix = glm::translate(modelMatrix, position);
    // Scale the box along each axis
    modelMatrix = glm::scale(modelMatrix, scale);
    // rotate around the axis
    //modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime(),rotateAxis);

    // TODO: Set model-view-projection matrix
    program.setMatrix("model", &modelMatrix[0][0]);
    program.setMatrix("view", &viewMatrix[0][0]);
    program.setMatrix("projection", &projectionMatrix[0][0]);
    // ------------------------------------
    //glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
    //program.setMatrix("MVP", &mvp[0][0]);
    // Draw the box
    glDrawElements(
        GL_TRIANGLES,      // mode
        36,    			   // number of indices
        GL_UNSIGNED_INT,   // type
        (void*)0           // element array buffer offset
    );

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

void Box::cleanup() {
    glDeleteBuffers(1, &vertexBufferID);
    glDeleteBuffers(1, &colorBufferID);
    glDeleteBuffers(1, &indexBufferID);
    glDeleteVertexArrays(1, &vertexArrayID);
}