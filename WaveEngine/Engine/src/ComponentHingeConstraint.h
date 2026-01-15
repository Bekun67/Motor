#pragma once

#include "ComponentConstraint.h"

// Hinge constraint - allows rotation around one axis (like a door hinge)
class ComponentHingeConstraint : public ComponentConstraint
{
public:
    ComponentHingeConstraint(GameObject* owner);
    ~ComponentHingeConstraint() override = default;

    void CreateConstraint() override;
    void UpdateConstraint() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Hinge axis (local space)
    void SetAxisA(const glm::vec3& axis);
    glm::vec3 GetAxisA() const { return axisA; }

    void SetAxisB(const glm::vec3& axis);
    glm::vec3 GetAxisB() const { return axisB; }

    // Angle limits
    void SetUseLimits(bool use);
    bool GetUseLimits() const { return useLimits; }

    void SetLimits(float low, float high);
    float GetLowLimit() const { return lowLimit; }
    float GetHighLimit() const { return highLimit; }

    // Motor
    void SetUseMotor(bool use);
    bool GetUseMotor() const { return useMotor; }

    void SetMotorVelocity(float velocity);
    float GetMotorVelocity() const { return motorVelocity; }

    void SetMotorMaxImpulse(float impulse);
    float GetMotorMaxImpulse() const { return motorMaxImpulse; }

    // Get current angle
    float GetCurrentAngle() const;

private:
    glm::vec3 axisA;
    glm::vec3 axisB;

    bool useLimits;
    float lowLimit;   // in radians
    float highLimit;  // in radians

    bool useMotor;
    float motorVelocity;
    float motorMaxImpulse;
};