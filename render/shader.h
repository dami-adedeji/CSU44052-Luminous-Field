#ifndef _SHADER_H_
#define _SHADER_H_

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Shader {
    GLuint ID;

    void initialise(const char *vertex_file_path, const char *fragment_file_path);
    void use() const;
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, glm::vec3 value) const;
    void setVec4(const std::string &name, glm::vec4 value) const;
    void setMatrix(const std::string &name, const float *value) const;
    void setMatrixArray(const std::string& name, const std::vector<glm::mat4>& matrices) const;
    void remove() const;
};
GLuint LoadShadersFromFile(const char *vertex_file_path, const char *fragment_file_path);

GLuint LoadShadersFromString(std::string VertexShaderCode, std::string FragmentShaderCode);

#endif
