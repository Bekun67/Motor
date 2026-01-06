#include "ComponentRigidBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Log.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

ComponentRigidBody::ComponentRigidBody(GameObject* owner)
    : Component(owner, ComponentType::RIGIDBODY),
    rigidBody(nullptr),
    collisionShape(nullptr),
    motionState(nullptr),
    mass(1.0f),
    isKinematic(false),
    scale(1.0f)
{
    name = "RigidBody";
    CreateRigidBody();
}

ComponentRigidBody::~ComponentRigidBody()
{
    DestroyRigidBody();
}

void ComponentRigidBody::CreateRigidBody()
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    // Get AABB from mesh component
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(owner->GetComponent(ComponentType::MESH));
    if (meshComp && meshComp->HasMesh())
    {
        glm::vec3 minBounds = meshComp->GetAABBMin();
        glm::vec3 maxBounds = meshComp->GetAABBMax();
        glm::vec3 size = (maxBounds - minBounds) * transform->GetScale();

        scale = transform->GetScale();

        // Create box collision shape
        collisionShape = new btBoxShape(btVector3(
            size.x * 0.5f,
            size.y * 0.5f,
            size.z * 0.5f
        ));
    }
    else
    {
        // Default box shape
        scale = transform->GetScale();
        collisionShape = new btBoxShape(btVector3(
            scale.x * 0.5f,
            scale.y * 0.5f,
            scale.z * 0.5f
        ));
    }

    // Create motion state
    glm::vec3 pos = transform->GetPosition();
    glm::quat rot = transform->GetRotationQuat();

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    startTransform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

    motionState = new btDefaultMotionState(startTransform);

    // Calculate inertia
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f && !isKinematic)
    {
        collisionShape->calculateLocalInertia(mass, localInertia);
    }

    // Create rigid body
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, collisionShape, localInertia);
    rigidBody = new btRigidBody(rbInfo);

    // Set kinematic flag
    if (isKinematic)
    {
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        rigidBody->setActivationState(DISABLE_DEACTIVATION);
    }

    // Store GameObject pointer for collision detection
    rigidBody->setUserPointer(owner);

    // Add to physics world
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld())
    {
        physics->GetDynamicsWorld()->addRigidBody(rigidBody);
    }
}

void ComponentRigidBody::DestroyRigidBody()
{
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld() && rigidBody)
    {
        physics->GetDynamicsWorld()->removeRigidBody(rigidBody);
    }

    delete rigidBody;
    delete motionState;
    delete collisionShape;

    rigidBody = nullptr;
    motionState = nullptr;
    collisionShape = nullptr;
}

void ComponentRigidBody::Update()
{
    if (!IsActive() || !rigidBody) return;

    // Sync physics transform to GameObject
    SyncTransformFromPhysics();
}

void ComponentRigidBody::SyncTransformFromPhysics()
{
    if (!rigidBody || isKinematic) return;

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    btTransform worldTrans;
    motionState->getWorldTransform(worldTrans);

    btVector3 origin = worldTrans.getOrigin();
    btQuaternion rotation = worldTrans.getRotation();

    transform->SetPosition(glm::vec3(origin.x(), origin.y(), origin.z()));
    transform->SetRotationQuat(glm::quat(rotation.w(), rotation.x(), rotation.y(), rotation.z()));
}

void ComponentRigidBody::SyncTransformToPhysics()
{
    if (!rigidBody) return;

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    glm::vec3 pos = transform->GetPosition();
    glm::quat rot = transform->GetRotationQuat();

    btTransform worldTrans;
    worldTrans.setOrigin(btVector3(pos.x, pos.y, pos.z));
    worldTrans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

    rigidBody->setWorldTransform(worldTrans);
    motionState->setWorldTransform(worldTrans);
}

void ComponentRigidBody::SetMass(float newMass)
{
    mass = newMass;

    if (rigidBody && collisionShape)
    {
        btVector3 localInertia(0, 0, 0);
        if (mass > 0.0f && !isKinematic)
        {
            collisionShape->calculateLocalInertia(mass, localInertia);
        }

        rigidBody->setMassProps(mass, localInertia);
        rigidBody->updateInertiaTensor();
    }
}

void ComponentRigidBody::SetKinematic(bool kinematic)
{
    isKinematic = kinematic;

    if (rigidBody)
    {
        if (isKinematic)
        {
            rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            rigidBody->setActivationState(DISABLE_DEACTIVATION);
        }
        else
        {
            rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
            rigidBody->setActivationState(ACTIVE_TAG);
        }
    }
}

void ComponentRigidBody::ApplyForce(const glm::vec3& force)
{
    if (rigidBody && !isKinematic)
    {
        rigidBody->activate();
        rigidBody->applyCentralForce(btVector3(force.x, force.y, force.z));
    }
}

void ComponentRigidBody::ApplyImpulse(const glm::vec3& impulse)
{
    if (rigidBody && !isKinematic)
    {
        rigidBody->activate();
        rigidBody->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
    }
}

glm::vec3 ComponentRigidBody::GetVelocity() const
{
    if (rigidBody)
    {
        btVector3 vel = rigidBody->getLinearVelocity();
        return glm::vec3(vel.x(), vel.y(), vel.z());
    }
    return glm::vec3(0.0f);
}

void ComponentRigidBody::SetVelocity(const glm::vec3& velocity)
{
    if (rigidBody)
    {
        rigidBody->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    }
}

void ComponentRigidBody::Serialize(nlohmann::json& componentObj) const
{
    componentObj["mass"] = mass;
    componentObj["isKinematic"] = isKinematic;
}

void ComponentRigidBody::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("mass"))
    {
        SetMass(componentObj["mass"].get<float>());
    }

    if (componentObj.contains("isKinematic"))
    {
        SetKinematic(componentObj["isKinematic"].get<bool>());
    }
}

void ComponentRigidBody::OnEditor()
{
    // To be implemented in InspectorWindow
}