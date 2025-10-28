#include "Camera.h"
#include "Application.h"
#include <iostream>

Camera::Camera(float fov, float aspect, float nearClip, float farClip)
    : position(0.0f, 1.0f, 3.0f),
    focusPoint(0.0f, 1.0f, 0.0f),
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
    lastY(0.0f),
    distanceToFocus(3.0f),
    orbitMode(false)
{
}

void Camera::HandleInput(float deltaTime)
{
    const bool* state = SDL_GetKeyboardState(NULL);
   

    bool altPressed = state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT];
    orbitMode = altPressed; 

    float mouseX, mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);

    // Free camera movement with right click
    if (!orbitMode && (buttons & SDL_BUTTON_RMASK))
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

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

        if (state[SDL_SCANCODE_W] || buttons & SDL_BUTTON_LEFT) position += front * moveSpeed * deltaTime;
        if (state[SDL_SCANCODE_S]) position -= front * moveSpeed * deltaTime;
        if (state[SDL_SCANCODE_A]) position -= right * moveSpeed * deltaTime;
        if (state[SDL_SCANCODE_D]) position += right * moveSpeed * deltaTime;
        if (state[SDL_SCANCODE_Q]) position.y -= moveSpeed * deltaTime;
        if (state[SDL_SCANCODE_E]) position.y += moveSpeed * deltaTime;

        // Actualizar el punto de enfoque (a donde está mirando)
        focusPoint = position + front * distanceToFocus;
    }
    // O rbital (Alt + Click derecho)
    else if (orbitMode && (buttons & SDL_BUTTON_RMASK))
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
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        position = focusPoint - glm::normalize(direction) * distanceToFocus;
    }
    else
    {
        firstMouse = true;
    }

    // Zoom with mouse Wheel input
    Zoom(deltaTime);

    // Frame selected ( F)
    if (state[SDL_SCANCODE_F])
    {
        FrameSelected(glm::vec3(0.0f, 0.0f, 0.0f)); // TODO: Use model position 
    }
}

void Camera::Zoom(float deltaTime)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_MOUSE_WHEEL)
        {
            // event.wheel.y -> positiva si la rueda se mueve hacia arriba / adelante
             // event.wheel.y -> negativa si se mueve hacia abajo / atrás
            float zoomSpeed = moveSpeed * deltaTime * 10.0f;

            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            front = glm::normalize(front);

            if (orbitMode)
            {
                // --- Zoom orbital (con Alt presionado)
                distanceToFocus -= event.wheel.y * zoomSpeed;
                if (distanceToFocus < minZoomDistance) distanceToFocus = minZoomDistance;
                if (distanceToFocus > maxZoomDistance) distanceToFocus = maxZoomDistance;

                position = focusPoint - front * distanceToFocus;
            }
            else
            {
                // --- Movimiento libre tipo Unity (sin Alt)
                if (event.wheel.y > 0)   // rueda hacia adelante
                    position += front * zoomSpeed;
                else if (event.wheel.y < 0) // rueda hacia atrás
                    position -= front * zoomSpeed;
            }
        }
    }
}

void Camera::FrameSelected(const glm::vec3& target, float distance)
{
    focusPoint = target;
    distanceToFocus = distance;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    position = focusPoint - glm::normalize(front) * distanceToFocus;
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
