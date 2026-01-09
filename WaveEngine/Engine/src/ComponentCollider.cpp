#include "ComponentCollider.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
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
    isTrigger(false),
    friction(0.5f),
    restitution(0.0f),
    showDebug(false)
{
    name = "Collider";
    CreateCollisionShape();
}

ComponentCollider::~ComponentCollider()
{
    DestroyCollisionShape();
}

void ComponentCollider::CreateCollisionShape()
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    //get world transform
    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

    glm::vec3 worldPosition;
    glm::quat worldRotation;
    glm::vec3 worldScale;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(globalMatrix, worldScale, worldRotation, worldPosition, skew, perspective);

    //get mesh bounds
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
            offsetPosition = meshCenter;
            break;
        case ColliderType::SPHERE:
            sphereRadius = glm::max(glm::max(meshSize.x, meshSize.y), meshSize.z) * 0.5f;
            offsetPosition = meshCenter;
            break;
        case ColliderType::CYLINDER:
            cylinderRadius = glm::max(meshSize.x, meshSize.z) * 0.5f;
            cylinderHeight = meshSize.z;
            offsetPosition = meshCenter;
            break;
        case ColliderType::CAPSULE:
            capsuleRadius = glm::max(meshSize.x, meshSize.z) * 0.5f;
            capsuleHeight = meshSize.z;
            offsetPosition = meshCenter;
            break;
        case ColliderType::PLANE:
            boxSize = meshSize;
            offsetPosition = meshCenter;
            planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            break;
        default:
            break;
        }
    }

    //create collision shape based on type
    switch (colliderType)
    {
    case ColliderType::BOX:
        collisionShape = new btBoxShape(btVector3(
            boxSize.x * 0.5f,
            boxSize.y * 0.5f,
            boxSize.z * 0.5f
        ));
        break;

    case ColliderType::SPHERE:
        collisionShape = new btSphereShape(sphereRadius);
        break;

    case ColliderType::CYLINDER:
        collisionShape = new btCylinderShape(btVector3(cylinderRadius, cylinderHeight * 0.5f, cylinderRadius));
        break;

    case ColliderType::CAPSULE:
        collisionShape = new btCapsuleShape(capsuleRadius, capsuleHeight);
        break;

    case ColliderType::PLANE:
        collisionShape = new btStaticPlaneShape(
            btVector3(planeNormal.x, planeNormal.y, planeNormal.z),
            0.0f
        );
        break;

    case ColliderType::MESH:
        LOG_DEBUG("TODO");
        collisionShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        break;
    }

    if (!collisionShape)
    {
        LOG_DEBUG("[ComponentCollider] Failed to create collision shape");
        return;
    }

    //apply offset to transform
    glm::mat4 rotationMatrix = glm::mat4_cast(worldRotation);
    glm::vec3 scaledOffset = offsetPosition * worldScale;
    glm::vec3 rotatedOffset = glm::vec3(rotationMatrix * glm::vec4(scaledOffset, 0.0f));
    glm::vec3 finalPosition = worldPosition + rotatedOffset;

    //create collision object
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(finalPosition.x, finalPosition.y, finalPosition.z));
    startTransform.setRotation(btQuaternion(worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w));

    collisionObject = new btCollisionObject();
    collisionObject->setCollisionShape(collisionShape);
    collisionObject->setWorldTransform(startTransform);

    collisionObject->setFriction(friction);
    collisionObject->setRestitution(restitution);

    //set trigger
    if (isTrigger)
    {
        collisionObject->setCollisionFlags(
            collisionObject->getCollisionFlags() |
            btCollisionObject::CF_NO_CONTACT_RESPONSE
        );
    }

    collisionObject->setUserPointer(owner);

    //add to physics world
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld())
    {
        physics->GetDynamicsWorld()->addCollisionObject(collisionObject);
    }

    LOG_DEBUG("[ComponentCollider] Created %s collider for '%s'",
        GetColliderTypeName().c_str(),
        owner->GetName().c_str());
}

void ComponentCollider::DestroyCollisionShape()
{
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld() && collisionObject)
    {
        physics->GetDynamicsWorld()->removeCollisionObject(collisionObject);
    }

    delete collisionObject;
    delete collisionShape;

    collisionObject = nullptr;
    collisionShape = nullptr;
}

void ComponentCollider::UpdateCollisionShape()
{
    DestroyCollisionShape();
    CreateCollisionShape();
}

void ComponentCollider::Update()
{
    if (!IsActive() || !collisionObject) return;

    SyncTransformToPhysics();
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

    // Apply offset
    glm::vec3 finalPosition = worldPosition + offsetPosition;

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
    offsetPosition = offset;
    SyncTransformToPhysics();
}

void ComponentCollider::SetIsTrigger(bool trigger)
{
    isTrigger = trigger;

    if (collisionObject)
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

    componentObj["offset"] = { offsetPosition.x, offsetPosition.y, offsetPosition.z };
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

    if (componentObj.contains("offset"))
    {
        auto& offset = componentObj["offset"];
        offsetPosition = glm::vec3(offset[0], offset[1], offset[2]);
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