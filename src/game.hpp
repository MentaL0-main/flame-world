#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <vector>
#include <GL/glew.h>
#include "logger.hpp"
#include "shader.hpp"
#include "controller.hpp"
#include "defaultController.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_obj_loader.h>

class Game
{
public:
    void run();

private:
    int windowWidth_ = 900;
    int windowHeight_ = 600;
    SDL_Window* window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;
    SDL_Event event_;
    bool running_ = true;

    Logger logger;

    void init();
    void mainLoop();
    void cleanUp();

    void createWindow();
    void initGLEW();
    void initRender();

    GLuint VAO = 0;
    GLuint VBO = 0;

    std::vector<float> plane = {
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
    };

    Shader shader;
    glm::mat4 MVP = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    GLuint MVP_LOC = 0;

    void matrixSetup();
    void acceptMatrix();

    bool controllerType = 0; // 0 - DController, 1 - Controller
    Controller controller;
    DController dController;

    float mouseX = 0.0f;
    float mouseY = 0.0f;

    struct MeshGL { GLuint vao=0, vbo=0, ebo=0; GLsizei idxCount=0; };

    bool loadObjCreateGL(const char* path, MeshGL &out);

    MeshGL cube;
};
