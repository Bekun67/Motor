#pragma once

#include "ComponentConstraint.h"

// Slider constraint - allows linear movement along one axis and rotation around it
class ComponentSliderConstraint : public ComponentConstraint
{
public:
    ComponentSliderConstraint(GameObject* owner);
    ~ComponentSliderConstraint() override = default;

    void CreateConstraint() override;
    void UpdateConstraint() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Slider axis (local space)
    void SetAxisA(const glm::vec3& axis);
    glm::vec3 GetAxisA() const { return axisA; }

    void SetAxisB(const glm::vec3& axis);
    glm::vec3 GetAxisB() const { return axisB; }

    // Linear limits
    void SetUseLinearLimits(bool use);
    bool GetUseLinearLimits() const { return useLinearLimits; }

    void SetLinearLimits(float lower, float upper);
    float GetLowerLinearLimit() const { return lowerLinearLimit; }
    float GetUpperLinearLimit() const { return upperLinearLimit; }

    // Angular limits
    void SetUseAngularLimits(bool use);
    bool GetUseAngularLimits() const { return useAngularLimits; }

    void SetAngularLimits(float lower, float upper);
    float GetLowerAngularLimit() const { return lowerAngularLimit; }
    float GetUpperAngularLimit() const { return upperAngularLimit; }

    // Get current position
    float GetCurrentLinearPosition() const;
    float GetCurrentAngularPosition() const;

private:
    glm::vec3 axisA;
    glm::vec3 axisB;

    bool useLinearLimits;
    float lowerLinearLimit;
    float upperLinearLimit;

    bool useAngularLimits;
    float lowerAngularLimit;  // in radians
    float upperAngularLimit;  // in radians
};