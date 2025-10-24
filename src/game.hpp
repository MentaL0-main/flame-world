#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <GL/glew.h>
#include "logger.hpp"
#include "model.hpp"
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

    Shader shader;
    glm::mat4 MVP = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    GLuint MVP_LOC     = 0;

    void matrixSetup();
    void acceptMatrix();

    bool controllerType = 0; // 0 - DController, 1 - Controller
    Controller controller;
    DController dController;

    float mouseX = 0.0f;
    float mouseY = 0.0f;

    Model home;

};
