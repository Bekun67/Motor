#pragma once

#include "ComponentConstraint.h"

// Distance constraint - maintains a fixed distance between two bodies
class ComponentDistanceConstraint : public ComponentConstraint
{
public:
    ComponentDistanceConstraint(GameObject* owner);
    ~ComponentDistanceConstraint() override = default;

    void CreateConstraint() override;
    void UpdateConstraint() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Distance settings
    void SetDistance(float distance);
    float GetDistance() const { return distance; }

    // Stiffness (0 = loose spring, 1 = rigid)
    void SetStiffness(float stiffness);
    float GetStiffness() const { return stiffness; }

    // Damping
    void SetDamping(float damping);
    float GetDamping() const { return damping; }

    // Min/Max distance limits (optional)
    void SetUseMinDistance(bool use);
    bool GetUseMinDistance() const { return useMinDistance; }

    void SetMinDistance(float minDist);
    float GetMinDistance() const { return minDistance; }

    void SetUseMaxDistance(bool use);
    bool GetUseMaxDistance() const { return useMaxDistance; }

    void SetMaxDistance(float maxDist);
    float GetMaxDistance() const { return maxDistance; }

    // Get current distance
    float GetCurrentDistance() const;

private:
    float distance;
    float stiffness;
    float damping;

    bool useMinDistance;
    float minDistance;

    bool useMaxDistance;
    float maxDistance;
};