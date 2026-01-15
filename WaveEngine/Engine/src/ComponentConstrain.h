#pragma once

#include "Component.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

class ComponentRigidBody;
class GameObject;

enum class ConstraintType {
    HINGE,
    DISTANCE,
    SLIDER,
    CONE
};

// Base class for all physics constraints
class ComponentConstraint : public Component
{
public:
    ComponentConstraint(GameObject* owner, ConstraintType type);
    virtual ~ComponentConstraint();

    void Enable() override;
    void Update() override;
    void Disable() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Constraint management
    virtual void CreateConstraint() = 0;
    virtual void DestroyConstraint();
    virtual void UpdateConstraint() = 0;

    // Connected body
    void SetConnectedBody(GameObject* otherBody);
    GameObject* GetConnectedBody() const { return connectedBody; }

    // Enable/Disable constraint
    void SetConstraintEnabled(bool enabled);
    bool IsConstraintEnabled() const { return constraintEnabled; }

    // Breaking settings
    void SetBreakingThreshold(float threshold);
    float GetBreakingThreshold() const { return breakingThreshold; }

    ConstraintType GetConstraintType() const { return constraintType; }

protected:
    // Helper to get RigidBody from GameObject
    ComponentRigidBody* GetRigidBody(GameObject* obj);

    btTypedConstraint* constraint;
    ConstraintType constraintType;

    GameObject* connectedBody;
    bool constraintEnabled;
    float breakingThreshold;

    // Anchor points (local space)
    glm::vec3 anchorPointA;
    glm::vec3 anchorPointB;

    bool needsRebuild;
};