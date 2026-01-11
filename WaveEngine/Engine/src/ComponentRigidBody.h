#pragma once

#include "Component.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

class ComponentCollider;

class ComponentRigidBody : public Component
{
public:
    ComponentRigidBody(GameObject* owner);
    ~ComponentRigidBody();

    void CreateRigidBody();
    void Enable() override;
    void Update() override;
    void Disable() override;
    void OnEditor() override;

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    void SetMass(float mass);
    float GetMass() const { return mass; }

    void SetKinematic(bool kinematic);
    bool IsKinematic() const { return isKinematic; }

    void ApplyForce(const glm::vec3& force);
    void ApplyImpulse(const glm::vec3& impulse);

    glm::vec3 GetVelocity() const;
    void SetVelocity(const glm::vec3& velocity);

    btRigidBody* GetBulletRigidBody() { return rigidBody; }
    btCompoundShape* GetCompoundShape() { return compoundShape; }
    void RecalculateInertia();

    void SyncTransformToPhysics();

private:
    void DestroyRigidBody();
    void SyncTransformFromPhysics();

    btRigidBody* rigidBody;
    btCompoundShape* compoundShape;
    btDefaultMotionState* motionState;

    float mass;
    bool isKinematic;
    glm::vec3 scale;
};