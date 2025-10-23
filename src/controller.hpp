#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

class Controller {
public:
    void init(float spd);
    void controlFree(const bool* keyboardState, glm::mat4& view, double delta, float mouseX, float mouseY);

private:
    float speed = 5.0f;
    float sensitivity = 10.0f;
    float pitch = 0.0f;
    float yaw = -90.0f;
    float xoffset = 0.0f;
    float yoffset = 0.0f;
    glm::vec3 position  = {0.0f, 0.5f, 3.0f};
    glm::vec3 front     = {0.0f, 0.0f, -1.0f};
    glm::vec3 up        = {0.0f, 1.0f, 0.0f};
    glm::vec3 direction = {0.0f, 0.0f, 0.0f};
};
