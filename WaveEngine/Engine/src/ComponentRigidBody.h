#pragma once

#include "Component.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

class ComponentRigidBody : public Component
{
public:
    ComponentRigidBody(GameObject* owner);
    ~ComponentRigidBody();

    void Update() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Physics properties
    void SetMass(float mass);
    float GetMass() const { return mass; }

    void SetKinematic(bool kinematic);
    bool IsKinematic() const { return isKinematic; }

    void ApplyForce(const glm::vec3& force);
    void ApplyImpulse(const glm::vec3& impulse);

    glm::vec3 GetVelocity() const;
    void SetVelocity(const glm::vec3& velocity);

private:
    void CreateRigidBody();
    void DestroyRigidBody();
    void SyncTransformFromPhysics();
    void SyncTransformToPhysics();

    btRigidBody* rigidBody;
    btCollisionShape* collisionShape;
    btDefaultMotionState* motionState;

    float mass;
    bool isKinematic;
    glm::vec3 scale;
};