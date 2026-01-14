#include "ComponentRigidBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentCollider.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Log.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

ComponentRigidBody::ComponentRigidBody(GameObject* owner)
    : Component(owner, ComponentType::RIGIDBODY),
    rigidBody(nullptr),
    compoundShape(nullptr),
    motionState(nullptr),
    mass(1.0f),
    isKinematic(false),
    scale(1.0f)
{
    CreateRigidBody();
    name = "RigidBody";
}

ComponentRigidBody::~ComponentRigidBody()
{
    DestroyRigidBody();
}

void ComponentRigidBody::Enable()
{
    CreateRigidBody();
}

void ComponentRigidBody::Disable()
{
    // notify all attached colliders that they're being detached
    std::vector<Component*> colliders = owner->GetComponentsOfType(ComponentType::COLLIDER);
    for (Component* comp : colliders)
    {
        ComponentCollider* collider = static_cast<ComponentCollider*>(comp);
        if (collider && collider->IsActive())
        {
            // Mark as not attached so they don't try to remove from compound
            collider->ForceStandaloneMode();
        }
    }

    DestroyRigidBody();
}

void ComponentRigidBody::CreateRigidBody()
{
    if (rigidBody != nullptr)
    {
        DestroyRigidBody();
    }

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    // Use global matrix for world position, rotation, and scale
    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

    // Decompose global matrix
    glm::vec3 worldPosition;
    glm::quat worldRotation;
    glm::vec3 worldScale;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(globalMatrix, worldScale, worldRotation, worldPosition, skew, perspective);

    scale = worldScale;

    // Create compound shape
    compoundShape = new btCompoundShape();

    // Add any existing colliders to the compound shape before creating the rigid body
    std::vector<Component*> existingColliders = owner->GetComponentsOfType(ComponentType::COLLIDER);
    for (Component* comp : existingColliders)
    {
        ComponentCollider* collider = static_cast<ComponentCollider*>(comp);
        if (collider && collider->IsActive())
        {
            // Get the collision shape from the collider
            btCollisionShape* shape = collider->GetCollisionShape();
            if (shape)
            {
				// Remove standalone collision object from world if it exists
                btCollisionObject* standaloneCO = collider->GetCollisionObject();
                if (standaloneCO)
                {
                    ModulePhysics* physics = Application::GetInstance().physics.get();
                    if (physics && physics->GetDynamicsWorld())
                    {
						// Verify that the standalone collision object is actually in the world before removing
                        btCollisionObjectArray& objectArray = physics->GetDynamicsWorld()->getCollisionObjectArray();
                        bool found = false;
                        for (int i = 0; i < objectArray.size(); i++)
                        {
                            if (objectArray[i] == standaloneCO)
                            {
                                found = true;
                                physics->GetDynamicsWorld()->removeCollisionObject(standaloneCO);
                                LOG_DEBUG("[ComponentRigidBody] Removed standalone collision object from world");
                                break;
                            }
                        }

                        if (!found)
                        {
                            LOG_DEBUG("[ComponentRigidBody] WARNING: Standalone collision object not in world");
                        }
                    }

					// Delete the standalone collision object
                    delete standaloneCO;

					//Force nullify the pointer in the collider
                    collider->ClearStandaloneObject();
                }

                // Add shape to compound
                btTransform localTransform;
                localTransform.setIdentity();
                glm::vec3 offset = collider->GetOffset();
                glm::vec3 scaledOffset = offset * worldScale;
                localTransform.setOrigin(btVector3(scaledOffset.x, scaledOffset.y, scaledOffset.z));

                compoundShape->addChildShape(localTransform, shape);

                LOG_DEBUG("[ComponentRigidBody] Added existing %s collider to compound shape",
                    collider->GetColliderTypeName().c_str());
            }
        }
    }

    // Create motion state
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(worldPosition.x, worldPosition.y, worldPosition.z));
    startTransform.setRotation(btQuaternion(worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w));

    motionState = new btDefaultMotionState(startTransform);

    // Calculate inertia
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f && !isKinematic)
    {
        compoundShape->calculateLocalInertia(mass, localInertia);
    }

    // Create rigid body
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, compoundShape, localInertia);
    rigidBody = new btRigidBody(rbInfo);

    // Set kinematic flag
    if (isKinematic)
    {
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        rigidBody->setActivationState(DISABLE_DEACTIVATION);
    }

    // Store gameobject pointer for collision detection
    rigidBody->setUserPointer(owner);

    // Add to physics world
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld())
    {
        physics->GetDynamicsWorld()->addRigidBody(rigidBody);
    }

    LOG_DEBUG("[ComponentRigidBody] Created for '%s' at world pos (%.2f, %.2f, %.2f) with %d shapes",
        owner->GetName().c_str(),
        worldPosition.x, worldPosition.y, worldPosition.z,
        compoundShape->getNumChildShapes());
}

void ComponentRigidBody::DestroyRigidBody()
{
	// notify all attached colliders that they're being detached
    std::vector<Component*> colliders = owner->GetComponentsOfType(ComponentType::COLLIDER);
    for (Component* comp : colliders)
    {
        ComponentCollider* collider = static_cast<ComponentCollider*>(comp);
        if (collider && collider->IsActive())
        {
			// Force collider to standalone mode so it doesn't try to remove from compound again
            collider->ForceStandaloneMode();
        }
    }

	// Remove from physics world
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld() && rigidBody)
    {
        physics->GetDynamicsWorld()->removeRigidBody(rigidBody);
        LOG_DEBUG("[ComponentRigidBody] Removed from physics world");
    }

	// Clear compound shapes
    if (compoundShape)
    {
        int numShapes = compoundShape->getNumChildShapes();
        for (int i = numShapes - 1; i >= 0; i--)
        {
            compoundShape->removeChildShapeByIndex(i);
        }
        LOG_DEBUG("[ComponentRigidBody] Removed %d child shapes from compound", numShapes);
    }

	// Delete Bullet objects
    if (rigidBody)
    {
        delete rigidBody;
        rigidBody = nullptr;
    }

    if (motionState)
    {
        delete motionState;
        motionState = nullptr;
    }

    if (compoundShape)
    {
        delete compoundShape;
        compoundShape = nullptr;
    }

    LOG_DEBUG("[ComponentRigidBody] Destroyed RigidBody completely");
}

void ComponentRigidBody::RecalculateInertia()
{
    if (!rigidBody || !compoundShape) return;

    btVector3 localInertia(0, 0, 0);

    // Only calculate inertia if there are shapes attached and mass > 0
    if (compoundShape->getNumChildShapes() > 0 && mass > 0.0f && !isKinematic)
    {
        compoundShape->calculateLocalInertia(mass, localInertia);
    }

    rigidBody->setMassProps(mass, localInertia);
    rigidBody->updateInertiaTensor();

    LOG_DEBUG("[RigidBody] Recalculated inertia for '%s' (shapes: %d)",
        owner->GetName().c_str(),
        compoundShape->getNumChildShapes());
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

    // Get the world transform from Bullet
    btTransform worldTrans;
    motionState->getWorldTransform(worldTrans);

    btVector3 origin = worldTrans.getOrigin();
    btQuaternion rotation = worldTrans.getRotation();

    glm::vec3 worldPosition(origin.x(), origin.y(), origin.z());
    glm::quat worldRotation(rotation.w(), rotation.x(), rotation.y(), rotation.z());

    // If it has a parent, convert from world to local
    GameObject* parent = owner->GetParent();
    if (parent)
    {
        Transform* parentTransform = static_cast<Transform*>(parent->GetComponent(ComponentType::TRANSFORM));
        if (parentTransform)
        {
            glm::mat4 parentGlobal = parentTransform->GetGlobalMatrix();
            glm::mat4 parentInverse = glm::inverse(parentGlobal);

            // Convert world position to local
            glm::vec4 localPos4 = parentInverse * glm::vec4(worldPosition, 1.0f);
            glm::vec3 localPosition(localPos4.x, localPos4.y, localPos4.z);

            // Extract parent rotation WITHOUT scale to avoid deformation
            glm::vec3 parentScale;
            glm::quat parentRotation;
            glm::vec3 parentTranslation;
            glm::vec3 skew;
            glm::vec4 perspective;

            glm::decompose(parentGlobal, parentScale, parentRotation, parentTranslation, skew, perspective);

            // Convert world rotation to local using only rotation 
            glm::quat localRotation = glm::inverse(parentRotation) * worldRotation;

            transform->SetPosition(localPosition);
            transform->SetRotationQuat(localRotation);
        }
        else
        {
            // Without parent transform, use world directly
            transform->SetPosition(worldPosition);
            transform->SetRotationQuat(worldRotation);
        }
    }
    else
    {
        // Without parent, set world directly
        transform->SetPosition(worldPosition);
        transform->SetRotationQuat(worldRotation);
    }
}

void ComponentRigidBody::SyncTransformToPhysics()
{
    if (!rigidBody) return;

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    // Get global matrix for world position, rotation, and scale
    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

    glm::vec3 worldPosition;
    glm::quat worldRotation;
    glm::vec3 worldScale;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(globalMatrix, worldScale, worldRotation, worldPosition, skew, perspective);

    // Apply world transform to Bullet
    btTransform worldTrans;
    worldTrans.setOrigin(btVector3(worldPosition.x, worldPosition.y, worldPosition.z));
    worldTrans.setRotation(btQuaternion(worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w));

    rigidBody->setWorldTransform(worldTrans);
    motionState->setWorldTransform(worldTrans);

    rigidBody->setLinearVelocity(btVector3(0, 0, 0));
    rigidBody->setAngularVelocity(btVector3(0, 0, 0));
    rigidBody->clearForces();
    rigidBody->activate(true);
}

void ComponentRigidBody::SetMass(float newMass)
{
    mass = newMass;

    if (rigidBody && compoundShape)
    {
        btVector3 localInertia(0, 0, 0);

        // Only calculate inertia if mass > 0 and not kinematic
        if (mass > 0.0f && !isKinematic)
        {
            compoundShape->calculateLocalInertia(mass, localInertia);
        }

        rigidBody->setMassProps(mass, localInertia);
        rigidBody->updateInertiaTensor();

        // Wake up the body so changes take effect immediately
        rigidBody->activate(true);

        LOG_DEBUG("[ComponentRigidBody] Mass set to %.2f for '%s'",
            mass, owner->GetName().c_str());
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
}