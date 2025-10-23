#include "defaultController.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include "defines.hpp"

void DController::init(float spd)
{
    speed = spd;
    yaw = 0.0f;
    std::cout << "[*] Success to init DController!\n";
}

void DController::controlFree(const bool* keyboardState, glm::mat4& view, double delta,
                              float mouseX, float mouseY)
{
    float dt = static_cast<float>(delta);

    xoffset = mouseX * sensitivity * dt;
    yoffset = mouseY * sensitivity * dt;

    yaw += xoffset;
    pitch -= yoffset;

    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    glm::vec3 forwardXZ = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 rightXZ   = glm::normalize(glm::cross(forwardXZ, up));

    glm::vec3 moveDir(0.0f);
    if (keyboardState[KEY_W]) moveDir += forwardXZ;
    if (keyboardState[KEY_S]) moveDir -= forwardXZ;
    if (keyboardState[KEY_A]) moveDir -= rightXZ;
    if (keyboardState[KEY_D]) moveDir += rightXZ;

    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir);
        position += moveDir * speed * dt;
        if (bobEnabled)
            bobTimer += dt * bobFrequency * glm::length(moveDir);
    }

    if (keyboardState[KEY_SPACE] && !isJumping) {
        isJumping = true;
        velocityY = jumpSpeed;
    }

    if (isJumping) {
        velocityY += gravity * dt;
        position.y += velocityY * dt;
    }

    float baseEye = floorY + eyeHeight;
    if (position.y <= baseEye) {
        position.y = baseEye;
        isJumping = false;
        velocityY = 0.0f;
    }

    float bobOffset = 0.0f;
    if (bobEnabled && glm::length(moveDir) > 0.0f)
        bobOffset = sin(bobTimer * glm::two_pi<float>()) * bobAmplitude;

    glm::vec3 eyePos = position;
    eyePos.y += bobOffset;

    view = glm::lookAt(eyePos, eyePos + front, up);
}
