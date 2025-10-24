#include "game.hpp"
#include <GL/glew.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include "defines.hpp"
#include <stdexcept>
#include <tiny_obj_loader.h>

void Game::run()
{
    init();
    mainLoop();
    cleanUp();
}

void Game::init()
{
    createWindow();
    initGLEW();
    initRender();
    matrixSetup();
    controller.init(2.0f);
    dController.init(2.0f);
    SDL_SetWindowRelativeMouseMode(window_, true);
}

void Game::mainLoop()
{
    Uint64 prev = SDL_GetPerformanceCounter();
    const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
    short count = 1000;

    while (running_)
    {
        while (SDL_PollEvent(&event_))
        {
            if (event_.type == SDL_EVENT_QUIT)
            {
                logger.message("Window is closed...");
                running_ = false;
            }

            if (event_.type == SDL_EVENT_MOUSE_MOTION)
            {
                mouseX = event_.motion.xrel;
                mouseY = event_.motion.yrel;
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double delta = (now - prev) / freq;
        prev = now;

        const bool* keyboardState = SDL_GetKeyboardState(NULL);
        if (keyboardState[KEY_ESCAPE]) running_ = false;
        if (keyboardState[KEY_0]) controllerType = 0;
        if (keyboardState[KEY_1]) controllerType = 1;

        if (controllerType == 0)
            dController.controlFree(reinterpret_cast<const bool*>(keyboardState), view, static_cast<float>(delta), mouseX, mouseY);
        else
            controller.controlFree(reinterpret_cast<const bool*>(keyboardState), view, static_cast<float>(delta), mouseX, mouseY);

        acceptMatrix();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        home.render(projection, view);

        SDL_GL_SwapWindow(window_);
        SDL_Delay(16);

        mouseX = 0; mouseY = 0;
        if (count < 700)
        {
            int fps = delta > 0.0 ? static_cast<int>(1.0 / delta) : 0;
            std::string fpsString = "Flame World: DEV (" + std::to_string(fps) + ')';
            SDL_SetWindowTitle(window_, fpsString.c_str());
            count = 1000;
        }
        --count;
    }
}

void Game::cleanUp()
{
    home.destroy();
    SDL_SetWindowRelativeMouseMode(window_, false);
    if (glContext_) SDL_GL_DestroyContext(glContext_), glContext_ = nullptr;
    if (window_) SDL_DestroyWindow(window_), window_ = nullptr;
    if (SDL_INIT_STATUS_INITIALIZED) SDL_Quit();
}

void Game::createWindow()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        logger.error("Failed to init the SDL3!");
        throw std::runtime_error(SDL_GetError());
    }
    logger.message("Success to init the SDL3!");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window_ = SDL_CreateWindow(
        "Flame World: Dev",
        windowWidth_,
        windowHeight_,
        SDL_WINDOW_OPENGL);
    if (!window_)
    {
        logger.error("Failed to create the window!");
        throw std::runtime_error(SDL_GetError());
    }
    logger.message("Success to create the window!");

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_)
    {
        logger.error("Failed to create the OpenGL Context!");
        throw std::runtime_error(SDL_GetError());
    }
    logger.message("Success to create the OpenGL Context!");

    SDL_GL_MakeCurrent(window_, glContext_);
}

void Game::initGLEW()
{
    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK)
    {
        logger.error("Failed to init the GLEW!");
        throw std::runtime_error(reinterpret_cast<const char*>(glewGetErrorString(glewStatus)));
    }
    logger.message("Success to init the GLEW!");
}

void Game::initRender()
{
    shader.loadSources("vertex.glsl", "fragment.glsl");
    shader.compile();
    shader.link();

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    home.init("./assets/casa.obj");
    home.setProgram(shader.getID());
}

void Game::matrixSetup()
{
    model = glm::mat4(1.0f);

    projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(windowWidth_) / static_cast<float>(windowHeight_),
        0.1f, 100.0f);

    MVP = projection * view * model;

    MVP_LOC = glGetUniformLocation(shader.getID(), "MVP");
}

void Game::acceptMatrix()
{
    MVP = projection * view * model;

    shader.use();
    glUniformMatrix4fv(MVP_LOC, 1, GL_FALSE, glm::value_ptr(MVP));

}
