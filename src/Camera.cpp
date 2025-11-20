#include "Camera.h"
#include "Application.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

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
    orbitMode(false),
    editorCamera(nullptr),
    editorCameraObject(nullptr),
    frustumCullingEnabled(false)
{
}

Camera::~Camera()
{
    if (editorCameraObject)
        delete editorCameraObject;
}

bool Camera::Start()
{
    // Create a GameObject for the editor camera
    editorCameraObject = new GameObject();
    editorCameraObject->name = "EditorCamera";

    // Add camera component
    editorCamera = new ComponentCamera(editorCameraObject);
    editorCameraObject->camera = editorCamera; // si tienes pointer camera en GameObject

    editorCamera->SetFOV(fov);
    editorCamera->SetAspectRatio(aspect);
    editorCamera->SetNearPlane(nearClip);
    editorCamera->SetFarPlane(farClip);

    return true;
}

bool Camera::CleanUp()
{
    if (editorCameraObject)
    {
        delete editorCameraObject;
        editorCameraObject = nullptr;
        editorCamera = nullptr;
    }
    return true;
}

void Camera::HandleInput(float deltaTime)
{
    const bool* state = SDL_GetKeyboardState(NULL);

    bool altPressed = state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT];
    orbitMode = altPressed;

    // Use shift to increase movement speed
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

        // Use currentSpeed and deltaTime to move camera
        if (state[SDL_SCANCODE_W] || buttons & SDL_BUTTON_LEFT) position += front * currentSpeed * deltaTime;

        if (state[SDL_SCANCODE_S]) position -= front * currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_A]) position -= right * currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_D]) position += right * currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_Q]) position.y -= currentSpeed * deltaTime;
        if (state[SDL_SCANCODE_E]) position.y += currentSpeed * deltaTime;

        // update focus point based on new position and front vector
        focusPoint = position + front * distanceToFocus;
    }
    // Orbital (Alt + Right click)
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

    ModuleEditor* moduleEditor = Application::GetInstance().editor.get();
    if (!moduleEditor->editing) {
        // Frame selected
        if (state[SDL_SCANCODE_F])
        {
            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (opengl->selectedGameObject == nullptr) return;

            float radius = glm::length(glm::vec3(
                opengl->selectedGameObject->transform->scaling.x,
                opengl->selectedGameObject->transform->scaling.y,
                opengl->selectedGameObject->transform->scaling.z
            )) * opengl->selectedGameObject->transform->radius;

            //we pass to the "FrameSelected" method the selectedGameObject's translation position and its radius
            FrameSelected(glm::vec3(opengl->selectedGameObject->transform->translation.x,
                opengl->selectedGameObject->transform->translation.y,
                opengl->selectedGameObject->transform->translation.z),
                radius);
        }
    }
    //update editor camera
    if (editorCameraObject && editorCameraObject->transform)
    {
        //transform
        editorCameraObject->transform->translation.x = position.x;
        editorCameraObject->transform->translation.y = position.y;
        editorCameraObject->transform->translation.z = position.z;

        //rotation
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(worldUp, front));
        glm::vec3 up = glm::cross(front, right);

        glm::mat3 rotMatrix;
        rotMatrix[0] = right;
        rotMatrix[1] = up;
        rotMatrix[2] = front;

        glm::quat rotation = glm::quat_cast(rotMatrix);

        editorCameraObject->transform->rotation.w = rotation.w;
        editorCameraObject->transform->rotation.x = rotation.x;
        editorCameraObject->transform->rotation.y = rotation.y;
        editorCameraObject->transform->rotation.z = rotation.z;
    }

    // Update camera component
    if (editorCamera)
    {
        editorCamera->SetAspectRatio(aspect);
        editorCamera->UpdateFrustum();
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

    //we change the focus distance depending on the selected game object's radius
    float fovRadians = glm::radians(fov);
    float distanceFromRadius = distance / tan(fovRadians * 0.5f);

    distanceToFocus = distanceFromRadius * 1.5f;

    distanceToFocus = glm::clamp(distanceToFocus, minZoomDistance, maxZoomDistance);

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    position = focusPoint - glm::normalize(front) * distanceToFocus;
}

glm::mat4 Camera::GetViewMatrix() const
{
    if (editorCamera) return editorCamera->GetViewMatrix();

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    if (editorCamera) return editorCamera->GetProjectionMatrix();

    return glm::perspective(glm::radians(fov), aspect, nearClip, farClip);
}