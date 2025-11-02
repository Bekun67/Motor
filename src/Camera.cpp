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

    // Detectar si SHIFT está pulsado para duplicar velocidad
    bool shiftPressed = state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT];
    float currentSpeed = shiftPressed ? moveSpeed * 2.0f : moveSpeed;

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

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

        // Usar currentSpeed en lugar de moveSpeed
        if (state[SDL_SCANCODE_W] || buttons & SDL_BUTTON_LEFT) position += front * currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_S]) position -= front * currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_A]) position -= right * currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_D]) position += right * currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_Q]) position.y -= currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_E]) position.y += currentSpeed * deltaTime;

        // update focus point based on new position and front vector
        focusPoint = position + front * distanceToFocus;
    }
    // Orbital (Alt + Click derecho)
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

    // Get delta of mouse wheel and call Zoom
    int scroll = Application::GetInstance().input.get()->GetMouseWheelY();
    Zoom((float)scroll, deltaTime);

    // Frame selected
    if (state[SDL_SCANCODE_F])
    {
        FrameSelected(glm::vec3(0.0f, 0.0f, 0.0f)); // TODO: Use model position 
    }
}

void Camera::Zoom(float scroll, float deltaTime)
{
    // Stop if no scroll or no deltaTime
    if (scroll == 0.0f || deltaTime == 0.0f)
        return;

    // Calculate front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    float scrollFactor = 5.0f;

    glm::vec3 movement = front * moveSpeed * deltaTime * scrollFactor;

    // scrol == 1 is zoom in
    if (scroll == 1.0f)
    {
        position += movement;
    }
    // scroll == -1 is zoom out
    else if (scroll == -1.0f)
    {
        position -= movement;
    }

    // Update distance to focus so we can keep the same distance
    focusPoint = position + front * distanceToFocus;
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