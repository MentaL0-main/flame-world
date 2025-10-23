#include "shader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    if (vertexPath && fragmentPath) loadSources(vertexPath, fragmentPath);
}

Shader::~Shader() {
    if (program) glDeleteProgram(program);
    deleteShaders();
}

void Shader::loadSources(const char* vertexPath, const char* fragmentPath) {
    vertexShaderSource = readFileToString(vertexPath);
    fragmentShaderSource = readFileToString(fragmentPath);
}

std::string Shader::readFileToString(const char* path) {
    std::ifstream ifs(path);
    if (!ifs) {
        throw std::runtime_error(std::string("Failed to open shader file: ") + (path ? path : "null"));
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

void Shader::compile() {
    deleteShaders();

    if (vertexShaderSource.empty() || fragmentShaderSource.empty())
        throw std::runtime_error("Shader source is empty. Call loadSources() first.");

    auto compile_one = [](GLenum type, const std::string& src) -> GLuint {
        GLuint s = glCreateShader(type);
        const char* cstr = src.c_str();
        glShaderSource(s, 1, &cstr, nullptr);
        glCompileShader(s);
        return s;
    };

    vertexShader = compile_one(GL_VERTEX_SHADER, vertexShaderSource);
    checkCompileErrors(vertexShader, "VERTEX");

    fragmentShader = compile_one(GL_FRAGMENT_SHADER, fragmentShaderSource);
    checkCompileErrors(fragmentShader, "FRAGMENT");
}

void Shader::link() {
    if (!vertexShader || !fragmentShader)
        throw std::runtime_error("Shaders not compiled before linking.");

    if (program) {
        glDeleteProgram(program);
        program = 0;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 0 ? len : 1);
        glGetProgramInfoLog(program, len, nullptr, log.data());
        std::string msg = "Program link error: " + std::string(log.data());
        glDeleteProgram(program);
        program = 0;
        throw std::runtime_error(msg);
    }

    deleteShaders();
}

void Shader::use() const {
    if (!program) throw std::runtime_error("Shader program not linked.");
    glUseProgram(program);
}

GLuint Shader::getID() const {
    return program;
}

void Shader::deleteShaders() noexcept {
    if (vertexShader) {
        glDeleteShader(vertexShader);
        vertexShader = 0;
    }
    if (fragmentShader) {
        glDeleteShader(fragmentShader);
        fragmentShader = 0;
    }
}

void Shader::checkCompileErrors(GLuint id, const std::string& type) {
    GLint success = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint len = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 0 ? len : 1);
        glGetShaderInfoLog(id, len, nullptr, log.data());
        std::string msg = type + " shader compile error: " + std::string(log.data());
        glDeleteShader(id);
        throw std::runtime_error(msg);
    }
}

Shader::Shader(Shader&& o) noexcept
    : vertexShaderSource(std::move(o.vertexShaderSource)),
      fragmentShaderSource(std::move(o.fragmentShaderSource)),
      vertexShader(o.vertexShader),
      fragmentShader(o.fragmentShader),
      program(o.program)
{
    o.vertexShader = o.fragmentShader = o.program = 0;
}

Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        if (program) glDeleteProgram(program);
        deleteShaders();

        vertexShaderSource = std::move(o.vertexShaderSource);
        fragmentShaderSource = std::move(o.fragmentShaderSource);
        vertexShader = o.vertexShader;
        fragmentShader = o.fragmentShader;
        program = o.program;

        o.vertexShader = o.fragmentShader = o.program = 0;
    }
    return *this;
}
