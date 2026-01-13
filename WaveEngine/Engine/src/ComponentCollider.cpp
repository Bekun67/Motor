#include "ComponentCollider.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Log.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

ComponentCollider::ComponentCollider(GameObject* owner, ColliderType type)
    : Component(owner, ComponentType::COLLIDER),
    collisionObject(nullptr),
    collisionShape(nullptr),
    colliderType(type),
    boxSize(1.0f, 1.0f, 1.0f),
    sphereRadius(0.5f),
    cylinderRadius(0.5f),
    cylinderHeight(1.0f),
    capsuleRadius(0.5f),
    capsuleHeight(1.0f),
    planeNormal(0.0f, 1.0f, 0.0f),
    offsetPosition(0.0f, 0.0f, 0.0f),
    internalOffset(0.0f, 0.0f, 0.0f),
    userOffset(0.0f, 0.0f, 0.0f),
    isTrigger(false),
    friction(0.5f),
    restitution(0.0f),
    showDebug(false),
    manuallyEdited(false),
    isAttachedToRigidBody(false)
{
    name = "Collider";
    CreateCollisionShape();
}

ComponentCollider::~ComponentCollider()
{
    DestroyCollisionShape();
}

void ComponentCollider::Enable()
{
    CreateCollisionShape();

    // If there's already a RigidBody, make sure we're properly attached
    ComponentRigidBody* rigidBody = static_cast<ComponentRigidBody*>(
        owner->GetComponent(ComponentType::RIGIDBODY)
        );

    if (rigidBody && rigidBody->IsActive() && !isAttachedToRigidBody)
    {
        // We need to rebuild to attach to the RigidBody
        UpdateCollisionShape();
    }
}

void ComponentCollider::Disable()
{
    // If attached to RigidBody, just mark as not attached
    if (isAttachedToRigidBody)
    {
        isAttachedToRigidBody = false;
        collisionShape = nullptr;
        collisionObject = nullptr;

        return;
    }

    // For standalone colliders, remove from physics world
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld() && collisionObject)
    {
        physics->GetDynamicsWorld()->removeCollisionObject(collisionObject);
        LOG_DEBUG("[ComponentCollider] Removed standalone collider from physics world");
    }

    // Clean up
    if (collisionObject)
    {
        delete collisionObject;
        collisionObject = nullptr;
    }

    if (collisionShape)
    {
        delete collisionShape;
        collisionShape = nullptr;
    }
}

void ComponentCollider::CreateCollisionShape()
{
    if (collisionShape != nullptr)
    {
        DestroyCollisionShape();
    }

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

    glm::vec3 worldPosition;
    glm::quat worldRotation;
    glm::vec3 worldScale;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(globalMatrix, worldScale, worldRotation, worldPosition, skew, perspective);

    ComponentMesh* meshComp = static_cast<ComponentMesh*>(owner->GetComponent(ComponentType::MESH));
    if (meshComp && meshComp->HasMesh() && !manuallyEdited)
    {
        glm::vec3 minBounds = meshComp->GetAABBMin();
        glm::vec3 maxBounds = meshComp->GetAABBMax();
        glm::vec3 meshSize = (maxBounds - minBounds);
        glm::vec3 meshCenter = (minBounds + maxBounds) * 0.5f;

        switch (colliderType)
        {
        case ColliderType::BOX:
            boxSize = meshSize;
            internalOffset = meshCenter;
            break;
        case ColliderType::SPHERE:
            sphereRadius = glm::max(glm::max(meshSize.x, meshSize.y), meshSize.z) * 0.5f;
            internalOffset = meshCenter;
            break;
        case ColliderType::CYLINDER:
            cylinderRadius = glm::max(meshSize.x, meshSize.y) * 0.5f;
            cylinderHeight = meshSize.z;
            internalOffset = meshCenter;
            break;
        case ColliderType::CAPSULE:
            capsuleRadius = glm::max(meshSize.x, meshSize.y) * 0.5f;
            capsuleHeight = meshSize.z;
            internalOffset = meshCenter;
            break;
        case ColliderType::PLANE:
            boxSize = meshSize;
            internalOffset = meshCenter;
            planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            break;
        default:
            break;
        }
    }

    // Create the collision shape based on type
    switch (colliderType)
    {
    case ColliderType::BOX:
        collisionShape = new btBoxShape(btVector3(
            boxSize.x * 0.5f * worldScale.x,
            boxSize.y * 0.5f * worldScale.y,
            boxSize.z * 0.5f * worldScale.z
        ));
        break;

    case ColliderType::SPHERE:
    {
        float uniformScale = glm::max(glm::max(worldScale.x, worldScale.y), worldScale.z);
        collisionShape = new btSphereShape(sphereRadius * uniformScale);
        break;
    }

    case ColliderType::CYLINDER:
    {
        btVector3 halfExtents(
            cylinderRadius * worldScale.x,
            cylinderRadius * worldScale.y,
            cylinderHeight * 0.5f * worldScale.z
        );
        collisionShape = new btCylinderShapeZ(halfExtents);
        break;
    }

    case ColliderType::CAPSULE:
    {
        float radialScale = glm::max(worldScale.x, worldScale.y);
        float scaledRadius = capsuleRadius * radialScale * 0.8;

        float halfHeight = capsuleHeight * 0.5f * worldScale.z;

        float cylinderHalfHeight = glm::max(0.01f, halfHeight - scaledRadius);

        collisionShape = new btCapsuleShapeZ(
            scaledRadius,
            cylinderHalfHeight * 2.0f * 1.2f
        );
        break;
    }

    case ColliderType::PLANE:
        collisionShape = new btStaticPlaneShape(
            btVector3(planeNormal.x, planeNormal.y, planeNormal.z),
            0.0f
        );
        break;

    case ColliderType::MESH:
        if (meshComp && meshComp->HasMesh())
        {
            const Mesh& mesh = meshComp->GetMesh();
            btConvexHullShape* convexHull = new btConvexHullShape();

            for (const auto& vertex : mesh.vertices)
            {
                convexHull->addPoint(btVector3(vertex.position.x, vertex.position.y, vertex.position.z), false);
            }

            convexHull->recalcLocalAabb();
            convexHull->setLocalScaling(btVector3(worldScale.x, worldScale.y, worldScale.z));
            convexHull->setMargin(0.04f);

            collisionShape = convexHull;

            LOG_DEBUG("[ComponentCollider] Created convex hull with %d vertices", mesh.vertices.size());
        }
        else
        {
            collisionShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        }
        break;
    }

    if (!collisionShape)
    {
        LOG_DEBUG("[ComponentCollider] Failed to create collision shape");
        return;
    }

    offsetPosition = internalOffset + userOffset;

    // Check if there's a rigidbody component
    ComponentRigidBody* rigidBody = static_cast<ComponentRigidBody*>(
        owner->GetComponent(ComponentType::RIGIDBODY)
        );

    if (rigidBody && rigidBody->IsActive())
    {
		// If we already had a standalone collider, remove it
        if (collisionObject)
        {
            ModulePhysics* physics = Application::GetInstance().physics.get();
            if (physics && physics->GetDynamicsWorld())
            {
                physics->GetDynamicsWorld()->removeCollisionObject(collisionObject);
                LOG_DEBUG("[ComponentCollider] Removed old standalone collider before attaching to RigidBody");
            }

            delete collisionObject;
            collisionObject = nullptr;
        }

        isAttachedToRigidBody = true;

        // Force RigidBody to rebuild and include this collider
        rigidBody->CreateRigidBody();

        LOG_DEBUG("[ComponentCollider] Created %s shape, triggering RigidBody rebuild",
            GetColliderTypeName().c_str());

        return;
    }

    // If there's NO RigidBody, create a standalone collision object
    isAttachedToRigidBody = false;

    glm::mat4 rotationMatrix = glm::mat4_cast(worldRotation);
    glm::vec3 scaledOffset = offsetPosition * worldScale;
    glm::vec3 rotatedOffset = glm::vec3(rotationMatrix * glm::vec4(scaledOffset, 0.0f));
    glm::vec3 finalPosition = worldPosition + rotatedOffset;

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(finalPosition.x, finalPosition.y, finalPosition.z));
    startTransform.setRotation(btQuaternion(worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w));

    collisionObject = new btCollisionObject();
    collisionObject->setCollisionShape(collisionShape);
    collisionObject->setWorldTransform(startTransform);
    collisionObject->setFriction(friction);
    collisionObject->setRestitution(restitution);

    if (isTrigger)
    {
        collisionObject->setCollisionFlags(
            collisionObject->getCollisionFlags() |
            btCollisionObject::CF_NO_CONTACT_RESPONSE
        );
    }

    collisionObject->setUserPointer(owner);

    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld())
    {
        physics->GetDynamicsWorld()->addCollisionObject(collisionObject);
    }

    LOG_DEBUG("[ComponentCollider] Created standalone %s collider for '%s'",
        GetColliderTypeName().c_str(),
        owner->GetName().c_str());
}

void ComponentCollider::RemoveFromRigidBody()
{
    if (!isAttachedToRigidBody || !collisionShape) return;

    ComponentRigidBody* rigidBody = static_cast<ComponentRigidBody*>(
        owner->GetComponent(ComponentType::RIGIDBODY)
        );

    if (rigidBody)
    {
        btCompoundShape* compoundShape = rigidBody->GetCompoundShape();
        if (compoundShape)
        {
            // Find and remove this specific shape from the compound
            for (int i = compoundShape->getNumChildShapes() - 1; i >= 0; i--)
            {
                if (compoundShape->getChildShape(i) == collisionShape)
                {
                    compoundShape->removeChildShapeByIndex(i);
                    LOG_DEBUG("[ComponentCollider] Removed shape from RigidBody (index: %d)", i);
                    break;
                }
            }

            // Recalculate inertia after removing shape
            rigidBody->RecalculateInertia();
        }
    }

    isAttachedToRigidBody = false;
}

void ComponentCollider::RemoveStandaloneFromWorld()
{
    // Remove standalone collision object from physics world
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld() && collisionObject)
    {
        physics->GetDynamicsWorld()->removeCollisionObject(collisionObject);
        LOG_DEBUG("[ComponentCollider] Removed standalone collider from physics world");
    }

    // Delete the standalone collision object
    if (collisionObject)
    {
        delete collisionObject;
        collisionObject = nullptr;
    }
}

void ComponentCollider::DestroyCollisionShape()
{
	// Verify if attached to RigidBody
    if (isAttachedToRigidBody)
    {
		// do not remove from rigid body here, as the rigid body handles it
        isAttachedToRigidBody = false;

		// Only delete the collision shape
        if (collisionShape)
        {
            delete collisionShape;
            collisionShape = nullptr;
        }

		// there is no collision object in this case
        collisionObject = nullptr;

        LOG_DEBUG("[ComponentCollider] Destroyed attached collision shape");
        return;
    }

	// For standalone colliders, remove from physics world
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld() && collisionObject)
    {
		// Assure the collision object is still in the world before removing
        btCollisionObjectArray& objectArray = physics->GetDynamicsWorld()->getCollisionObjectArray();
        bool found = false;
        for (int i = 0; i < objectArray.size(); i++)
        {
            if (objectArray[i] == collisionObject)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            physics->GetDynamicsWorld()->removeCollisionObject(collisionObject);
            LOG_DEBUG("[ComponentCollider] Removed standalone collider from physics world");
        }
        else
        {
            LOG_DEBUG("[ComponentCollider] WARNING: Collision object not found in world, skipping removal");
        }
    }

	// Delete collision object
    if (collisionObject)
    {
        delete collisionObject;
        collisionObject = nullptr;
    }

	// Delete collision shape
    if (collisionShape)
    {
        delete collisionShape;
        collisionShape = nullptr;
    }

    LOG_DEBUG("[ComponentCollider] Destroyed standalone collision shape");
}

void ComponentCollider::UpdateCollisionShape()
{
    DestroyCollisionShape();
    CreateCollisionShape();
}

void ComponentCollider::Update()
{
    if (!IsActive() || isAttachedToRigidBody) return;

    // Only sync transform if this is a standalone collider
    if (collisionObject)
    {
        SyncTransformToPhysics();
    }
}

void ComponentCollider::SyncTransformToPhysics()
{
    if (!collisionObject) return;

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

    glm::vec3 worldPosition;
    glm::quat worldRotation;
    glm::vec3 worldScale;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(globalMatrix, worldScale, worldRotation, worldPosition, skew, perspective);

    offsetPosition = internalOffset + userOffset;

    glm::mat4 rotationMatrix = glm::mat4_cast(worldRotation);
    glm::vec3 scaledOffset = offsetPosition * worldScale;
    glm::vec3 rotatedOffset = glm::vec3(rotationMatrix * glm::vec4(scaledOffset, 0.0f));
    glm::vec3 finalPosition = worldPosition + rotatedOffset;

    btTransform worldTrans;
    worldTrans.setOrigin(btVector3(finalPosition.x, finalPosition.y, finalPosition.z));
    worldTrans.setRotation(btQuaternion(worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w));

    collisionObject->setWorldTransform(worldTrans);
}

void ComponentCollider::SetColliderType(ColliderType type)
{
    if (colliderType != type)
    {
        colliderType = type;
        manuallyEdited = false;

        userOffset = glm::vec3(0.0f, 0.0f, 0.0f);
        internalOffset = glm::vec3(0.0f, 0.0f, 0.0f);

        UpdateCollisionShape();
        LOG_DEBUG("[ComponentCollider] Changed to %s for '%s'",
            GetColliderTypeName().c_str(),
            owner->GetName().c_str());
    }
}

std::string ComponentCollider::GetColliderTypeName() const
{
    switch (colliderType)
    {
    case ColliderType::BOX:     return "Box";
    case ColliderType::SPHERE:  return "Sphere";
    case ColliderType::CYLINDER:return "Cylinder";
    case ColliderType::CAPSULE: return "Capsule";
    case ColliderType::PLANE:   return "Plane";
    case ColliderType::MESH:    return "Mesh";
    default:                    return "Unknown";
    }
}

void ComponentCollider::SetBoxSize(const glm::vec3& size)
{
    boxSize = size;
    manuallyEdited = true;
    if (colliderType == ColliderType::BOX)
    {
        UpdateCollisionShape();
    }
}

void ComponentCollider::SetSphereRadius(float radius)
{
    sphereRadius = radius;
    manuallyEdited = true;
    if (colliderType == ColliderType::SPHERE)
    {
        UpdateCollisionShape();
    }
}

void ComponentCollider::SetCylinderSize(float radius, float height)
{
    cylinderRadius = radius;
    cylinderHeight = height;
    manuallyEdited = true;
    if (colliderType == ColliderType::CYLINDER)
    {
        UpdateCollisionShape();
    }
}

void ComponentCollider::SetCapsuleSize(float radius, float height)
{
    capsuleRadius = radius;
    capsuleHeight = height;
    manuallyEdited = true;
    if (colliderType == ColliderType::CAPSULE)
    {
        UpdateCollisionShape();
    }
}

void ComponentCollider::SetPlaneNormal(const glm::vec3& normal)
{
    planeNormal = glm::normalize(normal);
    manuallyEdited = true;
    if (colliderType == ColliderType::PLANE)
    {
        UpdateCollisionShape();
    }
}

void ComponentCollider::SetOffset(const glm::vec3& offset)
{
    userOffset = offset;
    UpdateCollisionShape();
}

void ComponentCollider::SetIsTrigger(bool trigger)
{
    isTrigger = trigger;

    if (isAttachedToRigidBody)
    {
        UpdateCollisionShape();
    }
    else if (collisionObject)
    {
        if (isTrigger)
        {
            collisionObject->setCollisionFlags(
                collisionObject->getCollisionFlags() |
                btCollisionObject::CF_NO_CONTACT_RESPONSE
            );
        }
        else
        {
            collisionObject->setCollisionFlags(
                collisionObject->getCollisionFlags() &
                ~btCollisionObject::CF_NO_CONTACT_RESPONSE
            );
        }
    }
}

void ComponentCollider::SetFriction(float newFriction)
{
    friction = newFriction;
    if (collisionObject)
    {
        collisionObject->setFriction(friction);
    }
}

void ComponentCollider::SetRestitution(float newRestitution)
{
    restitution = newRestitution;
    if (collisionObject)
    {
        collisionObject->setRestitution(restitution);
    }
}

void ComponentCollider::Serialize(nlohmann::json& componentObj) const
{
    componentObj["colliderType"] = static_cast<int>(colliderType);

    componentObj["boxSize"] = { boxSize.x, boxSize.y, boxSize.z };
    componentObj["sphereRadius"] = sphereRadius;
    componentObj["cylinderRadius"] = cylinderRadius;
    componentObj["cylinderHeight"] = cylinderHeight;
    componentObj["capsuleRadius"] = capsuleRadius;
    componentObj["capsuleHeight"] = capsuleHeight;
    componentObj["planeNormal"] = { planeNormal.x, planeNormal.y, planeNormal.z };

    componentObj["internalOffset"] = { internalOffset.x, internalOffset.y, internalOffset.z };
    componentObj["userOffset"] = { userOffset.x, userOffset.y, userOffset.z };
    componentObj["isTrigger"] = isTrigger;
    componentObj["friction"] = friction;
    componentObj["restitution"] = restitution;
    componentObj["showDebug"] = showDebug;
}

void ComponentCollider::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("colliderType"))
    {
        colliderType = static_cast<ColliderType>(componentObj["colliderType"].get<int>());
    }

    if (componentObj.contains("boxSize"))
    {
        auto& size = componentObj["boxSize"];
        boxSize = glm::vec3(size[0], size[1], size[2]);
    }

    if (componentObj.contains("sphereRadius"))
    {
        sphereRadius = componentObj["sphereRadius"].get<float>();
    }

    if (componentObj.contains("cylinderRadius"))
    {
        cylinderRadius = componentObj["cylinderRadius"].get<float>();
    }

    if (componentObj.contains("cylinderHeight"))
    {
        cylinderHeight = componentObj["cylinderHeight"].get<float>();
    }

    if (componentObj.contains("capsuleRadius"))
    {
        capsuleRadius = componentObj["capsuleRadius"].get<float>();
    }

    if (componentObj.contains("capsuleHeight"))
    {
        capsuleHeight = componentObj["capsuleHeight"].get<float>();
    }

    if (componentObj.contains("planeNormal"))
    {
        auto& normal = componentObj["planeNormal"];
        planeNormal = glm::vec3(normal[0], normal[1], normal[2]);
    }

    if (componentObj.contains("internalOffset"))
    {
        auto& offset = componentObj["internalOffset"];
        internalOffset = glm::vec3(offset[0], offset[1], offset[2]);
    }
    else
    {
        internalOffset = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    if (componentObj.contains("userOffset"))
    {
        auto& offset = componentObj["userOffset"];
        userOffset = glm::vec3(offset[0], offset[1], offset[2]);
    }
    else if (componentObj.contains("offset"))
    {
        auto& offset = componentObj["offset"];
        userOffset = glm::vec3(offset[0], offset[1], offset[2]);
    }
    else
    {
        userOffset = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    if (componentObj.contains("isTrigger"))
    {
        isTrigger = componentObj["isTrigger"].get<bool>();
    }

    if (componentObj.contains("friction"))
    {
        friction = componentObj["friction"].get<float>();
    }

    if (componentObj.contains("restitution"))
    {
        restitution = componentObj["restitution"].get<float>();
    }

    if (componentObj.contains("showDebug"))
    {
        showDebug = componentObj["showDebug"].get<bool>();
    }

    UpdateCollisionShape();
}