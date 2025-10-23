#include "controller.hpp"
#include "defines.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/common.hpp>
#include <iostream>
#include <GL/glew.h>

void Controller::init(float spd)
{
    speed = spd;
    std::cout << "[*] Success to init controller!\n";
}

void Controller::controlFree(const bool* keyboardState, glm::mat4& view, double delta,
                             float mouseX, float mouseY)
{
    direction.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    direction.y = sin(glm::radians(pitch));
    direction.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    xoffset = mouseX * sensitivity * static_cast<float>(delta);
    yoffset = mouseY * sensitivity * static_cast<float>(delta);

    yaw += xoffset;
    pitch -= yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    const float dt = static_cast<float>(delta);
    const glm::vec3 right = glm::normalize(glm::cross(front, up));
    if (keyboardState[KEY_W]) position += dt * speed * front;
    if (keyboardState[KEY_S]) position -= dt * speed * front;
    if (keyboardState[KEY_A]) position -= dt * speed * right;
    if (keyboardState[KEY_D]) position += dt * speed * right;
    if (keyboardState[KEY_LSHIFT]) position.y -= speed * dt;
    if (keyboardState[KEY_SPACE]) position.y += speed * dt;

    view = glm::lookAt(position, position + front, up);
}
