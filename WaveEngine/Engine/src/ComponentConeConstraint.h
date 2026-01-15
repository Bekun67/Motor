#pragma once

#include "ComponentConstraint.h"

// Cone constraint - allows rotation within a cone shape (like a ball-and-socket with limits)
class ComponentConeConstraint : public ComponentConstraint
{
public:
    ComponentConeConstraint(GameObject* owner);
    ~ComponentConeConstraint() override = default;

    void CreateConstraint() override;
    void UpdateConstraint() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Cone axis (local space)
    void SetAxisA(const glm::vec3& axis);
    glm::vec3 GetAxisA() const { return axisA; }

    void SetAxisB(const glm::vec3& axis);
    glm::vec3 GetAxisB() const { return axisB; }

    // Swing limits (cone angle limits)
    void SetUseSwingLimits(bool use);
    bool GetUseSwingLimits() const { return useSwingLimits; }

    void SetSwingSpan1(float angle);  // Swing limit around Y axis
    float GetSwingSpan1() const { return swingSpan1; }

    void SetSwingSpan2(float angle);  // Swing limit around Z axis
    float GetSwingSpan2() const { return swingSpan2; }

    // Twist limits (rotation around the cone axis)
    void SetUseTwistLimits(bool use);
    bool GetUseTwistLimits() const { return useTwistLimits; }

    void SetTwistSpan(float angle);
    float GetTwistSpan() const { return twistSpan; }

    // Softness, bias, and relaxation for limits
    void SetLimitSoftness(float softness);
    float GetLimitSoftness() const { return limitSoftness; }

    void SetLimitBias(float bias);
    float GetLimitBias() const { return limitBias; }

    void SetLimitRelaxation(float relaxation);
    float GetLimitRelaxation() const { return limitRelaxation; }

private:
    glm::vec3 axisA;
    glm::vec3 axisB;

    bool useSwingLimits;
    float swingSpan1;  // in radians
    float swingSpan2;  // in radians

    bool useTwistLimits;
    float twistSpan;   // in radians

    float limitSoftness;
    float limitBias;
    float limitRelaxation;
};