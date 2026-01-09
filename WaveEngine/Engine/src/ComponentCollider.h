#pragma once

#include "Component.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <string>

enum class ColliderType {
    BOX,
    SPHERE,
    CYLINDER,
    CAPSULE,
    PLANE,
    MESH
};

class ComponentCollider : public Component
{
public:
    ComponentCollider(GameObject* owner, ColliderType type = ColliderType::BOX);
    ~ComponentCollider();

    void Update() override;

    //serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    //collider types
    void SetColliderType(ColliderType type);
    ColliderType GetColliderType() const { return colliderType; }
    std::string GetColliderTypeName() const;

    //collider properties
    void SetBoxSize(const glm::vec3& size);
    glm::vec3 GetBoxSize() const { return boxSize; }

    void SetSphereRadius(float radius);
    float GetSphereRadius() const { return sphereRadius; }

    void SetCylinderSize(float radius, float height);
    float GetCylinderRadius() const { return cylinderRadius; }
    float GetCylinderHeight() const { return cylinderHeight; }

    void SetCapsuleSize(float radius, float height);
    float GetCapsuleRadius() const { return capsuleRadius; }
    float GetCapsuleHeight() const { return capsuleHeight; }

    void SetPlaneNormal(const glm::vec3& normal);
    glm::vec3 GetPlaneNormal() const { return planeNormal; }

    //offset
    void SetOffset(const glm::vec3& offset);
    glm::vec3 GetOffset() const { return offsetPosition; }

    //trigger mode
    void SetIsTrigger(bool trigger);
    bool IsTrigger() const { return isTrigger; }

    void SetFriction(float friction);
    float GetFriction() const { return friction; }

    void SetRestitution(float restitution);
    float GetRestitution() const { return restitution; }

    btCollisionObject* GetCollisionObject() const { return collisionObject; }

    //debug visualization
    void SetShowDebug(bool show) { showDebug = show; }
    bool GetShowDebug() const { return showDebug; }

    glm::vec3 GetUserOffset() const { return userOffset; }

private:
    void CreateCollisionShape();
    void DestroyCollisionShape();
    void UpdateCollisionShape();
    void SyncTransformToPhysics();

    //collision object
    btCollisionObject* collisionObject;
    btCollisionShape* collisionShape;

    //collider type
    ColliderType colliderType;

    //collider parameters
    glm::vec3 boxSize;
    float sphereRadius;
    float cylinderRadius;
    float cylinderHeight;
    float capsuleRadius;
    float capsuleHeight;
    glm::vec3 planeNormal;

    glm::vec3 offsetPosition;
    glm::vec3 internalOffset;
    glm::vec3 userOffset;

    bool isTrigger;

    float friction;
    float restitution;

    bool showDebug;
    bool manuallyEdited = false;
};