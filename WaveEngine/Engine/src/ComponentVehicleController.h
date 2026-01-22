#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class ComponentVehicleController : public Component
{
public:
    ComponentVehicleController(GameObject* owner);
    ~ComponentVehicleController();

    void Enable() override;
    void Update() override;
    void Disable() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Getters
    float GetAcceleration() const { return acceleration; }
    float GetMaxSpeed() const { return maxSpeed; }
    float GetTurnSpeed() const { return turnSpeed; }
    float GetBrakeForce() const { return brakeForce; }
    float GetDrag() const { return drag; }
    glm::vec3 GetForwardAxis() const { return forwardAxis; }

    // Setters
    void SetAcceleration(float accel) { acceleration = accel; }
    void SetMaxSpeed(float speed) { maxSpeed = speed; }
    void SetTurnSpeed(float speed) { turnSpeed = speed; }
    void SetBrakeForce(float force) { brakeForce = force; }
    void SetDrag(float dragValue) { drag = dragValue; }
    void SetForwardAxis(const glm::vec3& axis) { forwardAxis = glm::normalize(axis); }  // <-- NUEVO

private:
    void HandleInput();
    void ApplyMovement();

    // Vehicle physics properties
    float acceleration;
    float maxSpeed;
    float turnSpeed;
    float brakeForce;
    float drag;
    glm::vec3 forwardAxis;

    // Current state
    glm::vec3 currentVelocity;
    float currentSpeed;

    // Input state
    float forwardInput;
    float turnInput;
    bool isBraking;
};