#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class ComponentFirstPersonController : public Component
{
public:
    ComponentFirstPersonController(GameObject* owner);
    ~ComponentFirstPersonController();

    void Enable() override;
    void Update() override;
    void Disable() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Getters
    float GetMovementSpeed() const { return movementSpeed; }
    float GetShootForce() const { return shootForce; }
    float GetSphereSize() const { return sphereSize; }
    float GetMouseSensitivity() const { return mouseSensitivity; }
    float GetColliderRadius() const { return colliderRadius; }

    // Setters
    void SetMovementSpeed(float speed) { movementSpeed = speed; }
    void SetShootForce(float force) { shootForce = force; }
    void SetSphereSize(float size) { sphereSize = size; }
    void SetMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    void SetColliderRadius(float radius);

private:
    void HandleMovement();
    void HandleMouseLook();
    void ShootSphere();

    // Configuration
    void CreatePlayerCollider();
    void CreatePlayerRigidBody();
    
    float movementSpeed;
    float shootForce;
    float sphereSize;
    float mouseSensitivity;
    float colliderRadius;

    // Mouse control
    float yaw;
    float pitch;
    bool firstMouse;
    float lastMouseX;
    float lastMouseY;
    bool isRightMousePressed;
};