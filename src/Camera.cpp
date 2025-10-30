#include "Camera.h"
#include "Application.h"
#include <iostream>

Camera::Camera(float fov, float aspect, float nearClip, float farClip)
    : position(0.0f, 1.0f, 3.0f),
    yaw(-90.0f),
    pitch(0.0f),
    moveSpeed(5.0f),
    mouseSensitivity(0.1f),
    fov(fov),
    aspect(aspect),
    nearClip(nearClip),
    farClip(farClip),
    firstMouse(true),
    lastX(0.0f),
    lastY(0.0f)
{
}

void Camera::HandleInput(float deltaTime)
{
    const bool* state = SDL_GetKeyboardState(NULL);
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

	// Movement keyboard (Unity style)
    if (state[SDL_SCANCODE_W])
        position += front * moveSpeed * deltaTime;
    if (state[SDL_SCANCODE_S])
        position -= front * moveSpeed * deltaTime;
    if (state[SDL_SCANCODE_A])
        position -= right * moveSpeed * deltaTime;
    if (state[SDL_SCANCODE_D])
        position += right * moveSpeed * deltaTime;
    if (state[SDL_SCANCODE_Q])
        position.y -= moveSpeed * deltaTime;
    if (state[SDL_SCANCODE_E])
        position.y += moveSpeed * deltaTime;

    // Mouse rotation movement
    float mouseX, mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);


	if (buttons & SDL_BUTTON_RMASK) // Rotate only if right mouse button is held
    {
        if (firstMouse)
        {
            lastX = mouseX;
            lastY = mouseY;
            firstMouse = false;
        }

        float xoffset = mouseX - lastX;
        float yoffset = lastY - mouseY; 
        lastX = mouseX;
        lastY = mouseY;

        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }
    else
    {
        firstMouse = true;
    }
}

glm::mat4 Camera::GetViewMatrix() const
{
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    return glm::perspective(glm::radians(fov), aspect, nearClip, farClip);
}
