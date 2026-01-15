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
    CreateConstraint();
}

void ComponentConstraint::Update()
{
    if (!IsActive()) return;

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
            physics->GetDynamicsWorld()->removeConstraint(constraint);
            LOG_DEBUG("[ComponentConstraint] Removed constraint from physics world");
        }

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

    // Save connected body reference (by name for now)
    if (connectedBody)
    {
        componentObj["connectedBodyName"] = connectedBody->GetName();
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

    // TODO: Resolve connected body reference after scene load
}