#include "ComponentDistanceConstraint.h"
#include "GameObject.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Transform.h"
#include "Log.h"

ComponentDistanceConstraint::ComponentDistanceConstraint(GameObject* owner)
    : ComponentConstraint(owner, ConstraintType::DISTANCE),
    distance(1.0f),
    stiffness(1.0f),
    damping(0.1f),
    useMinDistance(false),
    minDistance(0.0f),
    useMaxDistance(false),
    maxDistance(10.0f)
{
    name = "Distance Constraint";
}

void ComponentDistanceConstraint::CreateConstraint()
{
    if (constraint != nullptr)
    {
        DestroyConstraint();
    }

    ComponentRigidBody* rbA = GetRigidBody(owner);
    if (!rbA)
    {
        LOG_DEBUG("[ComponentDistanceConstraint] Owner '%s' has no RigidBody component", owner->GetName().c_str());
        return;
    }

    if (!rbA->IsActive())
    {
        LOG_DEBUG("[ComponentDistanceConstraint] Owner '%s' RigidBody is not active", owner->GetName().c_str());
        return;
    }

    btRigidBody* bodyA = rbA->GetBulletRigidBody();
    if (!bodyA)
    {
        LOG_DEBUG("[ComponentDistanceConstraint] Owner '%s' has no Bullet RigidBody", owner->GetName().c_str());
        return;
    }

    if (!connectedBody)
    {
        LOG_DEBUG("[ComponentDistanceConstraint] No connected body specified");
        return;
    }

    ComponentRigidBody* rbB = GetRigidBody(connectedBody);
    if (!rbB)
    {
        LOG_DEBUG("[ComponentDistanceConstraint] Connected body '%s' has no RigidBody component", connectedBody->GetName().c_str());
        return;
    }

    if (!rbB->IsActive())
    {
        LOG_DEBUG("[ComponentDistanceConstraint] Connected body '%s' RigidBody is not active", connectedBody->GetName().c_str());
        return;
    }

    btRigidBody* bodyB = rbB->GetBulletRigidBody();
    if (!bodyB)
    {
        LOG_DEBUG("[ComponentDistanceConstraint] Connected body '%s' has no Bullet RigidBody", connectedBody->GetName().c_str());
        return;
    }

    // Create point-to-point constraint with anchor points
    btVector3 pivotA(anchorPointA.x, anchorPointA.y, anchorPointA.z);
    btVector3 pivotB(anchorPointB.x, anchorPointB.y, anchorPointB.z);

    btPoint2PointConstraint* p2pConstraint = new btPoint2PointConstraint(
        *bodyA, *bodyB,
        pivotA, pivotB
    );

    // Configure constraint parameters
    p2pConstraint->m_setting.m_damping = damping;
    p2pConstraint->m_setting.m_impulseClamp = stiffness * 100.0f;
    p2pConstraint->m_setting.m_tau = 0.3f;  // Error correction parameter

    // Set breaking threshold
    if (breakingThreshold > 0.0f)
    {
        p2pConstraint->setBreakingImpulseThreshold(breakingThreshold);
    }

    // Enable/disable
    p2pConstraint->setEnabled(constraintEnabled);

    // Add to physics world
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (physics && physics->GetDynamicsWorld())
    {
        physics->GetDynamicsWorld()->addConstraint(p2pConstraint, true);
    }

    constraint = p2pConstraint;

    LOG_DEBUG("[ComponentDistanceConstraint] Created distance constraint between '%s' and '%s'",
        owner->GetName().c_str(),
        connectedBody->GetName().c_str());
}

void ComponentDistanceConstraint::UpdateConstraint()
{
    if (!constraint) return;

    btPoint2PointConstraint* p2pConstraint = static_cast<btPoint2PointConstraint*>(constraint);

    // Update damping and stiffness
    p2pConstraint->m_setting.m_damping = damping;
    p2pConstraint->m_setting.m_impulseClamp = stiffness * 100.0f;
}

void ComponentDistanceConstraint::SetDistance(float dist)
{
    distance = dist;

    // Calculate new anchor points based on distance
    if (connectedBody)
    {
        Transform* transformA = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
        Transform* transformB = static_cast<Transform*>(connectedBody->GetComponent(ComponentType::TRANSFORM));

        if (transformA && transformB)
        {
            glm::vec3 posA = transformA->GetPosition();
            glm::vec3 posB = transformB->GetPosition();
            glm::vec3 direction = glm::normalize(posB - posA);

            // Set anchor points to maintain the desired distance
            anchorPointA = glm::vec3(0.0f);
            anchorPointB = direction * distance;

            needsRebuild = true;
        }
    }
}

void ComponentDistanceConstraint::SetStiffness(float stiff)
{
    stiffness = glm::clamp(stiff, 0.0f, 1.0f);
    UpdateConstraint();
}

void ComponentDistanceConstraint::SetDamping(float damp)
{
    damping = glm::clamp(damp, 0.0f, 1.0f);
    UpdateConstraint();
}

void ComponentDistanceConstraint::SetUseMinDistance(bool use)
{
    useMinDistance = use;
}

void ComponentDistanceConstraint::SetMinDistance(float minDist)
{
    minDistance = minDist;
}

void ComponentDistanceConstraint::SetUseMaxDistance(bool use)
{
    useMaxDistance = use;
}

void ComponentDistanceConstraint::SetMaxDistance(float maxDist)
{
    maxDistance = maxDist;
}

float ComponentDistanceConstraint::GetCurrentDistance() const
{
    if (!constraint || !connectedBody) return 0.0f;

    Transform* transformA = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    Transform* transformB = static_cast<Transform*>(connectedBody->GetComponent(ComponentType::TRANSFORM));

    if (transformA && transformB)
    {
        glm::vec3 posA = transformA->GetPosition();
        glm::vec3 posB = transformB->GetPosition();
        return glm::length(posB - posA);
    }

    return 0.0f;
}

void ComponentDistanceConstraint::OnEditor()
{
    // This will be called from InspectorWindow
}

void ComponentDistanceConstraint::Serialize(nlohmann::json& componentObj) const
{
    ComponentConstraint::Serialize(componentObj);

    componentObj["distance"] = distance;
    componentObj["stiffness"] = stiffness;
    componentObj["damping"] = damping;

    componentObj["useMinDistance"] = useMinDistance;
    componentObj["minDistance"] = minDistance;

    componentObj["useMaxDistance"] = useMaxDistance;
    componentObj["maxDistance"] = maxDistance;
}

void ComponentDistanceConstraint::Deserialize(const nlohmann::json& componentObj)
{
    ComponentConstraint::Deserialize(componentObj);

    if (componentObj.contains("distance"))
        distance = componentObj["distance"].get<float>();

    if (componentObj.contains("stiffness"))
        stiffness = componentObj["stiffness"].get<float>();

    if (componentObj.contains("damping"))
        damping = componentObj["damping"].get<float>();

    if (componentObj.contains("useMinDistance"))
        useMinDistance = componentObj["useMinDistance"].get<bool>();

    if (componentObj.contains("minDistance"))
        minDistance = componentObj["minDistance"].get<float>();

    if (componentObj.contains("useMaxDistance"))
        useMaxDistance = componentObj["useMaxDistance"].get<bool>();

    if (componentObj.contains("maxDistance"))
        maxDistance = componentObj["maxDistance"].get<float>();
}