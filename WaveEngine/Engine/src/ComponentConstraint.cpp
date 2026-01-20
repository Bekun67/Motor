#include "ComponentConstraint.h"
#include "GameObject.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Log.h"

ComponentConstraint::ComponentConstraint(GameObject* owner, ConstraintType type)
    : Component(owner, ComponentType::CONSTRAINT),
    constraint(nullptr),
    constraintType(type),
    connectedBody(nullptr),
    constraintEnabled(true),
    breakingThreshold(0.0f),
    anchorPointA(0.0f),
    anchorPointB(0.0f),
    needsRebuild(false)
{
    name = "Constraint";
}

ComponentConstraint::~ComponentConstraint()
{
    DestroyConstraint();
}

void ComponentConstraint::Enable()
{
    Application::PlayState playState = Application::GetInstance().GetPlayState();
    if (playState != Application::PlayState::EDITING)
    {
        CreateConstraint();
    }
    else
    {
        LOG_DEBUG("[ComponentConstraint] Constraint enabled but not created (EDITING mode)");
    }
}

void ComponentConstraint::Update()
{
    if (!IsActive()) return;

    if (!IsConstraintValid())
    {
        LOG_DEBUG("[ComponentConstraint] Constraint no longer valid on '%s', attempting to fix",
            owner->GetName().c_str());

        if (connectedBody)
        {
            ComponentRigidBody* rb = GetRigidBody(connectedBody);
            if (!rb || !rb->IsActive())
            {
                OnConnectedBodyInvalidated();
            }
        }

        ComponentRigidBody* ownRb = GetRigidBody(owner);
        if (!ownRb || !ownRb->IsActive())
        {
            LOG_DEBUG("[ComponentConstraint] Own RigidBody invalid, disabling constraint");
            SetActive(false);
            return;
        }
    }

    if (needsRebuild)
    {
        DestroyConstraint();
        CreateConstraint();
        needsRebuild = false;
    }

    // Check if constraint is broken
    if (constraint && breakingThreshold > 0.0f)
    {
        if (constraint->isEnabled() == false)
        {
            LOG_DEBUG("[ComponentConstraint] Constraint broken for '%s'",
                owner->GetName().c_str());
            SetActive(false);
        }
    }
}

void ComponentConstraint::Disable()
{
    DestroyConstraint();
}

void ComponentConstraint::DestroyConstraint()
{
    if (constraint)
    {
        ModulePhysics* physics = Application::GetInstance().physics.get();
        if (physics && physics->GetDynamicsWorld())
        {
            btDynamicsWorld* world = physics->GetDynamicsWorld();

			// verify that the constraint is actually in the world before removing
            bool found = false;
            for (int i = 0; i < world->getNumConstraints(); i++)
            {
                if (world->getConstraint(i) == constraint)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                world->removeConstraint(constraint);
                LOG_DEBUG("[ComponentConstraint] Removed constraint from physics world");
            }
            else
            {
                LOG_DEBUG("[ComponentConstraint] Constraint not in world, skipping removal");
            }
        }

		// Ensure it's disabled
        constraint->setEnabled(false);

        delete constraint;
        constraint = nullptr;
    }
}

void ComponentConstraint::SetConnectedBody(GameObject* otherBody)
{
    if (connectedBody != otherBody)
    {
        connectedBody = otherBody;
        needsRebuild = true;
    }
}

void ComponentConstraint::SetConstraintEnabled(bool enabled)
{
    constraintEnabled = enabled;

    if (constraint)
    {
        constraint->setEnabled(enabled);
    }
}

void ComponentConstraint::SetBreakingThreshold(float threshold)
{
    breakingThreshold = threshold;

    if (constraint && threshold > 0.0f)
    {
        constraint->setBreakingImpulseThreshold(threshold);
    }
}

ComponentRigidBody* ComponentConstraint::GetRigidBody(GameObject* obj)
{
    if (!obj) return nullptr;

    return static_cast<ComponentRigidBody*>(
        obj->GetComponent(ComponentType::RIGIDBODY)
        );
}

const ComponentRigidBody* ComponentConstraint::GetRigidBody(GameObject* obj) const
{
    if (!obj) return nullptr;

    return static_cast<const ComponentRigidBody*>(
        obj->GetComponent(ComponentType::RIGIDBODY)
        );
}

void ComponentConstraint::OnConnectedBodyInvalidated()
{
    LOG_DEBUG("[ComponentConstraint] Connected body invalidated for constraint on '%s'",
        owner->GetName().c_str());

    connectedBody = nullptr;
    DestroyConstraint();
}

bool ComponentConstraint::IsConstraintValid() const
{
    if (connectedBody)
    {
        const ComponentRigidBody* rb = GetRigidBody(connectedBody);
        if (!rb || !rb->IsActive())
        {
            return false;
        }
    }

    const ComponentRigidBody* ownRb = GetRigidBody(owner);
    if (!ownRb || !ownRb->IsActive())
    {
        return false;
    }

    return true;
}

void ComponentConstraint::Serialize(nlohmann::json& componentObj) const
{
    componentObj["constraintType"] = static_cast<int>(constraintType);
    componentObj["constraintEnabled"] = constraintEnabled;
    componentObj["breakingThreshold"] = breakingThreshold;

    componentObj["anchorPointA"] = {
        anchorPointA.x, anchorPointA.y, anchorPointA.z
    };
    componentObj["anchorPointB"] = {
        anchorPointB.x, anchorPointB.y, anchorPointB.z
    };

    if (connectedBody)
    {
        int bodyIndex = connectedBody->GetSerializationIndex();
        if (bodyIndex >= 0)
        {
            componentObj["connectedBodyIndex"] = bodyIndex;
        }
    }
}

void ComponentConstraint::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("constraintEnabled"))
    {
        constraintEnabled = componentObj["constraintEnabled"].get<bool>();
    }

    if (componentObj.contains("breakingThreshold"))
    {
        breakingThreshold = componentObj["breakingThreshold"].get<float>();
    }

    if (componentObj.contains("anchorPointA"))
    {
        auto& anchor = componentObj["anchorPointA"];
        anchorPointA = glm::vec3(anchor[0], anchor[1], anchor[2]);
    }

    if (componentObj.contains("anchorPointB"))
    {
        auto& anchor = componentObj["anchorPointB"];
        anchorPointB = glm::vec3(anchor[0], anchor[1], anchor[2]);
    }

    if (componentObj.contains("connectedBodyIndex"))
    {
        pendingConnectedBodyIndex = componentObj["connectedBodyIndex"].get<int>();
    }
}

void ComponentConstraint::ResolveConnectedBodyReference(const std::vector<GameObject*>& allGameObjects)
{
    if (pendingConnectedBodyIndex < 0)
    {
        return;
    }

    if (pendingConnectedBodyIndex >= 0 && pendingConnectedBodyIndex < static_cast<int>(allGameObjects.size()))
    {
        connectedBody = allGameObjects[pendingConnectedBodyIndex];
        LOG_DEBUG("[ComponentConstraint] Resolved connected body at index %d for constraint on '%s'",
            pendingConnectedBodyIndex, owner->GetName().c_str());
    }
    else
    {
        LOG_DEBUG("[ComponentConstraint] WARNING: Invalid connected body index %d for constraint on '%s'",
            pendingConnectedBodyIndex, owner->GetName().c_str());
    }

    pendingConnectedBodyIndex = -1;
}