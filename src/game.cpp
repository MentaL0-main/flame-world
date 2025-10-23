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
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstdint>
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

        glBindVertexArray(VAO);
        shader.use();
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glBindVertexArray(cube.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cube.idxCount), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

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
    SDL_SetWindowRelativeMouseMode(window_, false);
    if (glContext_) SDL_GL_DestroyContext(glContext_), glContext_ = nullptr;
    if (window_) SDL_DestroyWindow(window_), window_ = nullptr;
    if (SDL_INIT_STATUS_INITIALIZED) SDL_Quit();
}

void Game::createWindow()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
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
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, plane.size() * sizeof(GLfloat), plane.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader.loadSources("vertex.glsl", "fragment.glsl");
    shader.compile();
    shader.link();

    if (!loadObjCreateGL("./assets/cube.obj", cube))
    {
        std::cerr << "[!] Failed to load obj file. cube\n";
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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

bool Game::loadObjCreateGL(const char* path, MeshGL &out)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) return false;

    struct V { float px,py,pz; float nx,ny,nz; float u,v; };
    std::vector<V> verts;
    std::vector<uint32_t> inds;
    verts.reserve(256); inds.reserve(256);
    std::unordered_map<uint64_t,uint32_t> dedup;
    dedup.reserve(256);

    auto pack = [](int a,int b,int c)->uint64_t{
        return (uint64_t(static_cast<uint32_t>(a))<<32) | (uint64_t(static_cast<uint16_t>(b & 0xFFFF))<<16) | uint64_t(static_cast<uint16_t>(c & 0xFFFF));
    };

    for (const auto &shape : shapes)
    {
        for (const auto &idx : shape.mesh.indices)
        {
            uint64_t key = pack(idx.vertex_index, idx.normal_index + 1, idx.texcoord_index + 1);
            auto it = dedup.find(key);
            if (it != dedup.end()) { inds.push_back(it->second); continue; }
            V v{};
            int vi = idx.vertex_index * 3;
            v.px = attrib.vertices[vi + 0]; v.py = attrib.vertices[vi + 1]; v.pz = attrib.vertices[vi + 2];
            if (idx.normal_index >= 0) { int ni = idx.normal_index * 3; v.nx = attrib.normals[ni + 0]; v.ny = attrib.normals[ni + 1]; v.nz = attrib.normals[ni + 2]; }
            else { v.nx = 0.0f; v.ny = 0.0f; v.nz = 1.0f; }
            if (idx.texcoord_index >= 0) { int ti = idx.texcoord_index * 2; v.u = attrib.texcoords[ti + 0]; v.v = attrib.texcoords[ti + 1]; }
            else { v.u = 0.0f; v.v = 0.0f; }
            uint32_t newIdx = static_cast<uint32_t>(verts.size());
            verts.push_back(v);
            dedup.emplace(key, newIdx);
            inds.push_back(newIdx);
        }
    }

    if (out.vao == 0) glGenVertexArrays(1, &out.vao);
    if (out.vbo == 0)  glGenBuffers(1, &out.vbo);
    if (out.ebo == 0)  glGenBuffers(1, &out.ebo);

    glBindVertexArray(out.vao);
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(V), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(uint32_t), inds.data(), GL_STATIC_DRAW);

    GLsizei stride = static_cast<GLsizei>(sizeof(V));
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(V, px)));
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(V, nx)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(V, u)));

    glBindVertexArray(0);
    out.idxCount = static_cast<GLsizei>(inds.size());
    return true;
}
