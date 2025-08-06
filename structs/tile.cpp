#include "tile.h"
#include "texture.h"
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

const GLfloat Tile::vertices[12] = {
    // to have position as center rather than front left:
    -0.5f, 0.0f, 0.5f, // front left
    0.5f, 0.0f, 0.5f, // front right
    0.5f, 0.0f, -0.5f, // back right
    -0.5f, 0.0f, -0.5f // back left
};

const GLfloat Tile::normals[12] = {
    0.0f, 1.0f, 0.0f, // front left
    0.0f, 1.0f, 0.0f, // front right
    0.0f, 1.0f, 0.0f, // back right
    0.0f, 1.0f, 0.0f  // back left
};

const GLfloat Tile::colours[12] = {
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f
};

const GLfloat Tile::uv[8] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
};

const GLint Tile::indices[6] = {
    0, 1, 2,
    0, 2, 3
};

void Tile::initialise(glm::vec3 position, Shader program, const char* texture_path)
{
    this->position = position;
    hasTexture = true;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &NBO);
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);

    glGenBuffers(1, &CBO);
    glBindBuffer(GL_ARRAY_BUFFER, CBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

    glGenBuffers(1, &UVBO);
    glBindBuffer(GL_ARRAY_BUFFER, UVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    if (texture_path == nullptr || texture_path[0] == '\0')
    {
        hasTexture = false;
        // Create a vertex buffer object to store the color data
        glGenBuffers(1, &CBO);
        glBindBuffer(GL_ARRAY_BUFFER, CBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);
    }
    else
    {
        hasTexture = true;
        textureID = LoadTextureTileBox(texture_path);
        if (!glIsTexture(textureID))
            std::cerr << "Invalid texture loaded from " << texture_path << std::endl;
    }

    /*normalID = LoadTextureTileBox(normal_path);
    if (!glIsTexture(normalID))
        std::cerr << "Invalid texture loaded from " << texture_path << std::endl;*/
}

void Tile::render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Shader &program)
{
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    if (hasTexture == false) {
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, CBO);
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
    //mvpMatrixID = glGetUniformLocation(shaderID, "MVP");

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);  // Apply translation
    modelMatrix = glm::scale(modelMatrix, glm::vec3(tileSize, 1, tileSize));
    // Calculate MVP matrix


    // Send MVP matrix to the shader
    //glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &MVP[0][0]);
    //program.setMatrix("MVP", &mvp[0][0]);
    program.setMatrix("model", &modelMatrix[0][0]);

    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, UVBO);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);

    /* set normal sampler to use texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalID);
    program.setInt("normalSampler", 1);*/

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
}

void Tile::renderDepth(Shader& program)
{
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(tileSize, 1, tileSize));
    program.setMatrix("model", &modelMatrix[0][0]);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Tile::cleanup()
{
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &CBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &UVBO);
    glDeleteVertexArrays(1,&VAO);
}