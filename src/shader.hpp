#pragma once

#include <GL/glew.h>
#include <string>

class Shader
{
public:
    Shader() = default;
    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();

    Shader(Shader&&) noexcept;
    Shader& operator=(Shader&&) noexcept;

    void loadSources(const char* vertexPath, const char* fragmentPath);
    void compile();
    void link();
    void use() const;
    GLuint getID() const;

private:
    std::string readFileToString(const char* path);
    void deleteShaders() noexcept;
    void checkCompileErrors(GLuint id, const std::string& type);

    std::string vertexShaderSource;
    std::string fragmentShaderSource;
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    GLuint program = 0;
};
