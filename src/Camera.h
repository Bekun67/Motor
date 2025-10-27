#pragma once
#include "Module.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

class Camera : public Module
{
public:
    Camera(float fov = 60.0f, float aspect = 16.0f / 9.0f, float nearClip = 0.1f, float farClip = 100.0f);

    void HandleInput(float deltaTime);  // Movimiento estilo Unity
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    glm::vec3 GetPosition() const { return position; }

private:
    glm::vec3 position;
    float yaw;
    float pitch;

    float moveSpeed;
    float mouseSensitivity;

    float fov;
    float aspect;
    float nearClip;
    float farClip;

    bool firstMouse;
    float lastX, lastY;
};
