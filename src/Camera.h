#pragma once
#include "Module.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

class Camera : public Module
{
public:
    Camera(float fov = 60.0f, float aspect = 16.0f / 9.0f, float nearClip = 0.1f, float farClip = 100.0f);

    void HandleInput(float deltaTime);  // Movement
   void Zoom(float scrollY, float deltaTime); // Zoom input
    void FrameSelected(const glm::vec3& target, float distance = 5.0f); // Select focus point of an object with F
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetFocusPoint() const { return focusPoint; }

private:
    glm::vec3 position;
    glm::vec3 focusPoint;
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

    
    float distanceToFocus;  
    float minZoomDistance = 1.0f;
    float maxZoomDistance = 50.0f;
    bool orbitMode;
};
