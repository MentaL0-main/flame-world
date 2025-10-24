#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

class Model {
public:
    Model();
    ~Model();

    // Инициализация: путь к .obj (автоматически ищет .mtl и текстуры рядом)
    // Возвращает true при успехе
    bool init(const std::string &objPath);

    // Трансформации (накопительные)
    void translate(const glm::vec3 &t);
    void rotate(float angleRadians, const glm::vec3 &axis);
    void scale(const glm::vec3 &s);

    // Установить шейдерную программу (программа должна иметь uniforms: MVP, model,
    // uAlbedo (sampler2D), uUseTex (int), uColor, uLightDir, uAmbient)
    void setProgram(GLuint program);

    // Рендер — использует текущие projection/view заданные глобально извне через setPV
    void render(const glm::mat4 &projection, const glm::mat4 &view);

    // Освободить GPU ресурсы
    void destroy();

    // Удобства
    bool valid() const { return vao_ != 0; }
    void setColor(const glm::vec3 &color);

private:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    // internal helpers
    bool loadObj(const std::string &path);
    GLuint loadTextureFromFile(const std::string &path);
    void ensureProgramUniforms();

    // GPU
    GLuint vao_{0}, vbo_{0}, ibo_{0};
    size_t indexCount_{0};

    // materials: for each range store texture id (0 if none) and index range
    struct MatRange { GLuint texID; size_t start, count; glm::vec3 color; bool useTex; };
    std::vector<MatRange> materials_;

    // CPU-side storage (only during init)
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;

    // transform
    glm::mat4 modelMat_{1.0f};

    // shader program + uniform locations
    GLuint program_{0};
    GLint loc_MVP_{-1}, loc_model_{-1}, loc_uAlbedo_{-1}, loc_uUseTex_{-1},
          loc_uColor_{-1}, loc_uLightDir_{-1}, loc_uAmbient_{-1};
};
