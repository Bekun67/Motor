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

    void Enable() override;
    void Update() override;
    void Disable() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Collider types
    void SetColliderType(ColliderType type);
    ColliderType GetColliderType() const { return colliderType; }
    std::string GetColliderTypeName() const;

    // Collider properties
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

    // Offset
    void SetOffset(const glm::vec3& offset);
    glm::vec3 GetOffset() const { return offsetPosition; }

    // Trigger mode
    void SetIsTrigger(bool trigger);
    bool IsTrigger() const { return isTrigger; }

    void SetFriction(float friction);
    float GetFriction() const { return friction; }

    void SetRestitution(float restitution);
    float GetRestitution() const { return restitution; }

    btCollisionObject* GetCollisionObject() const { return collisionObject; }
    btCollisionShape* GetCollisionShape() const { return collisionShape; }

    // Debug visualization
    void SetShowDebug(bool show) { showDebug = show; }
    bool GetShowDebug() const { return showDebug; }

    glm::vec3 GetUserOffset() const { return userOffset; }

    void UpdateCollisionShape();

    void ForceStandaloneMode() { isAttachedToRigidBody = false; }

private:
    void CreateCollisionShape();
    void DestroyCollisionShape();
    void SyncTransformToPhysics();
    void RemoveFromRigidBody();
	void RemoveStandaloneFromWorld();

    // Collision object
    btCollisionObject* collisionObject;
    btCollisionShape* collisionShape;

    // Collider type
    ColliderType colliderType;

    // Collider parameters
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

    bool isAttachedToRigidBody = false;

    friend class ComponentRigidBody;  

	// Force clearing the standalone collision object without deleting it
    void ClearStandaloneObject()
    {
        collisionObject = nullptr;
        isAttachedToRigidBody = true;
    }
};