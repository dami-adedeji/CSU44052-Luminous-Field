#ifndef _SHADER_H_
#define _SHADER_H_

#include <glad/gl.h>
#include <string>

struct Shader {
    GLuint ID;

    void initialise(const char *vertex_file_path, const char *fragment_file_path);
    void use() const;
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int v0) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, float v0, float v1, float v2) const;
    void setMatrix(const std::string &name, const float *value) const;
    void remove() const;
};
GLuint LoadShadersFromFile(const char *vertex_file_path, const char *fragment_file_path);

GLuint LoadShadersFromString(std::string VertexShaderCode, std::string FragmentShaderCode);

#endif
