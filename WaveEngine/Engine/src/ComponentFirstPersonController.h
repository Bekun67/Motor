#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <vector>

struct ProjectileInfo
{
    GameObject* gameObject;
    float timeAlive;

    ProjectileInfo(GameObject* obj) : gameObject(obj), timeAlive(0.0f) {}
};

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
    float GetProjectileLifetime() const { return projectileLifetime; }

    // Setters
    void SetMovementSpeed(float speed) { movementSpeed = speed; }
    void SetShootForce(float force) { shootForce = force; }
    void SetSphereSize(float size) { sphereSize = size; }
    void SetMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    void SetColliderRadius(float radius);
    void SetProjectileLifetime(float lifetime) { projectileLifetime = lifetime; }

private:
    void HandleMovement();
    void HandleMouseLook();
    void ShootSphere();
    void UpdateProjectiles(); 

    // Configuration
    void CreatePlayerCollider();
    void CreatePlayerRigidBody();

    float movementSpeed;
    float shootForce;
    float sphereSize;
    float mouseSensitivity;
    float colliderRadius;
    float projectileLifetime; 

    // Mouse control
    float yaw;
    float pitch;
    bool firstMouse;
    float lastMouseX;
    float lastMouseY;
    bool isRightMousePressed;

    // Projectile tracking
    std::vector<ProjectileInfo> activeProjectiles;
};