#include "ComponentConeConstraint.h"
#include "GameObject.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Log.h"

ComponentConeConstraint::ComponentConeConstraint(GameObject* owner)
    : ComponentConstraint(owner, ConstraintType::CONE),
    axisA(0.0f, 1.0f, 0.0f),
    axisB(0.0f, 1.0f, 0.0f),
    useSwingLimits(true),
    swingSpan1(glm::pi<float>() / 4.0f),  // 45 degrees
    swingSpan2(glm::pi<float>() / 4.0f),  // 45 degrees
    useTwistLimits(true),
    twistSpan(glm::pi<float>() / 2.0f),   // 90 degrees
    limitSoftness(0.9f),
    limitBias(0.3f),
    limitRelaxation(1.0f)
{
    name = "Cone Constraint";
}

void ComponentConeConstraint::CreateConstraint()
{
    if (constraint != nullptr)
    {
        DestroyConstraint();
    }

    ComponentRigidBody* rbA = GetRigidBody(owner);
    if (!rbA)
    {
        LOG_DEBUG("[ComponentConeConstraint] Owner '%s' has no RigidBody component", owner->GetName().c_str());
        return;
    }

    if (!rbA->IsActive())
    {
        LOG_DEBUG("[ComponentConeConstraint] Owner '%s' RigidBody is not active", owner->GetName().c_str());
        return;
    }

    btRigidBody* bodyA = rbA->GetBulletRigidBody();
    if (!bodyA)
    {
        LOG_DEBUG("[ComponentConeConstraint] Owner '%s' has no Bullet RigidBody", owner->GetName().c_str());
        return;
    }

    // Create frame transforms
    btTransform frameInA;
    frameInA.setIdentity();
    frameInA.setOrigin(btVector3(anchorPointA.x, anchorPointA.y, anchorPointA.z));

    // Cone twist axis (Y-axis in local frame by default)
    btVector3 twistAxis(axisA.x, axisA.y, axisA.z);
    twistAxis.normalize();

    // Create orthonormal basis with twist axis as Y
    btVector3 xAxis, zAxis;
    btPlaneSpace1(twistAxis, xAxis, zAxis);

    btMatrix3x3 rotationMatrix(
        xAxis.x(), twistAxis.x(), zAxis.x(),
        xAxis.y(), twistAxis.y(), zAxis.y(),
        xAxis.z(), twistAxis.z(), zAxis.z()
    );

    frameInA.setBasis(rotationMatrix);

    btConeTwistConstraint* coneConstraint = nullptr;

    if (connectedBody)
    {
        ComponentRigidBody* rbB = GetRigidBody(connectedBody);
        if (!rbB)
        {
            LOG_DEBUG("[ComponentConeConstraint] Connected body '%s' has no RigidBody component", connectedBody->GetName().c_str());
            return;
        }

        if (!rbB->IsActive())
        {
            LOG_DEBUG("[ComponentConeConstraint] Connected body '%s' RigidBody is not active", connectedBody->GetName().c_str());
            return;
        }

        btRigidBody* bodyB = rbB->GetBulletRigidBody();
        if (!bodyB)
        {
            LOG_DEBUG("[ComponentConeConstraint] Connected body '%s' has no Bullet RigidBody", connectedBody->GetName().c_str());
            return;
        }

        btTransform frameInB;
        frameInB.setIdentity();
        frameInB.setOrigin(btVector3(anchorPointB.x, anchorPointB.y, anchorPointB.z));

        btVector3 twistAxisB(axisB.x, axisB.y, axisB.z);
        twistAxisB.normalize();

        btVector3 xAxisB, zAxisB;
        btPlaneSpace1(twistAxisB, xAxisB, zAxisB);

        btMatrix3x3 rotationMatrixB(
            xAxisB.x(), twistAxisB.x(), zAxisB.x(),
            xAxisB.y(), twistAxisB.y(), zAxisB.y(),
            xAxisB.z(), twistAxisB.z(), zAxisB.z()
        );

        frameInB.setBasis(rotationMatrixB);

        coneConstraint = new btConeTwistConstraint(
            *bodyA, *bodyB,
            frameInA, frameInB
        );

        LOG_DEBUG("[ComponentConeConstraint] Created cone constraint between '%s' and '%s'",
            owner->GetName().c_str(),
            connectedBody->GetName().c_str());
    }
    else
    {
        coneConstraint = new btConeTwistConstraint(
            *bodyA,
            frameInA
        );

        LOG_DEBUG("[ComponentConeConstraint] Created cone constraint for '%s' (attached to world)",
            owner->GetName().c_str());
    }

    if (coneConstraint)
    {
        // Set swing limits
        if (useSwingLimits && useTwistLimits)
        {
            coneConstraint->setLimit(swingSpan1, swingSpan2, twistSpan,
                limitSoftness, limitBias, limitRelaxation);
        }
        else if (useSwingLimits && !useTwistLimits)
        {
            coneConstraint->setLimit(swingSpan1, swingSpan2, BT_LARGE_FLOAT,
                limitSoftness, limitBias, limitRelaxation);
        }
        else if (!useSwingLimits && useTwistLimits)
        {
            coneConstraint->setLimit(BT_LARGE_FLOAT, BT_LARGE_FLOAT, twistSpan,
                limitSoftness, limitBias, limitRelaxation);
        }
        else
        {
            // No limits
            coneConstraint->setLimit(BT_LARGE_FLOAT, BT_LARGE_FLOAT, BT_LARGE_FLOAT);
        }

        // Set breaking threshold
        if (breakingThreshold > 0.0f)
        {
            coneConstraint->setBreakingImpulseThreshold(breakingThreshold);
        }

        // Enable/disable
        coneConstraint->setEnabled(constraintEnabled);

        // Add to physics world
        ModulePhysics* physics = Application::GetInstance().physics.get();
        if (physics && physics->GetDynamicsWorld())
        {
            physics->GetDynamicsWorld()->addConstraint(coneConstraint, true);
        }

        constraint = coneConstraint;
    }
}

void ComponentConeConstraint::UpdateConstraint()
{
    if (!constraint) return;

    btConeTwistConstraint* coneConstraint = static_cast<btConeTwistConstraint*>(constraint);

    // Update limits
    if (useSwingLimits && useTwistLimits)
    {
        coneConstraint->setLimit(swingSpan1, swingSpan2, twistSpan,
            limitSoftness, limitBias, limitRelaxation);
    }
    else if (useSwingLimits && !useTwistLimits)
    {
        coneConstraint->setLimit(swingSpan1, swingSpan2, BT_LARGE_FLOAT,
            limitSoftness, limitBias, limitRelaxation);
    }
    else if (!useSwingLimits && useTwistLimits)
    {
        coneConstraint->setLimit(BT_LARGE_FLOAT, BT_LARGE_FLOAT, twistSpan,
            limitSoftness, limitBias, limitRelaxation);
    }
    else
    {
        coneConstraint->setLimit(BT_LARGE_FLOAT, BT_LARGE_FLOAT, BT_LARGE_FLOAT);
    }
}

void ComponentConeConstraint::SetAxisA(const glm::vec3& axis)
{
    axisA = glm::normalize(axis);
    needsRebuild = true;
}

void ComponentConeConstraint::SetAxisB(const glm::vec3& axis)
{
    axisB = glm::normalize(axis);
    needsRebuild = true;
}

void ComponentConeConstraint::SetUseSwingLimits(bool use)
{
    useSwingLimits = use;
    UpdateConstraint();
}

void ComponentConeConstraint::SetSwingSpan1(float angle)
{
    swingSpan1 = angle;
    UpdateConstraint();
}

void ComponentConeConstraint::SetSwingSpan2(float angle)
{
    swingSpan2 = angle;
    UpdateConstraint();
}

void ComponentConeConstraint::SetUseTwistLimits(bool use)
{
    useTwistLimits = use;
    UpdateConstraint();
}

void ComponentConeConstraint::SetTwistSpan(float angle)
{
    twistSpan = angle;
    UpdateConstraint();
}

void ComponentConeConstraint::SetLimitSoftness(float softness)
{
    limitSoftness = glm::clamp(softness, 0.0f, 1.0f);
    UpdateConstraint();
}

void ComponentConeConstraint::SetLimitBias(float bias)
{
    limitBias = glm::clamp(bias, 0.0f, 1.0f);
    UpdateConstraint();
}

void ComponentConeConstraint::SetLimitRelaxation(float relaxation)
{
    limitRelaxation = glm::clamp(relaxation, 0.0f, 1.0f);
    UpdateConstraint();
}

void ComponentConeConstraint::OnEditor()
{
    // This will be called from InspectorWindow
}

void ComponentConeConstraint::Serialize(nlohmann::json& componentObj) const
{
    ComponentConstraint::Serialize(componentObj);

    componentObj["axisA"] = { axisA.x, axisA.y, axisA.z };
    componentObj["axisB"] = { axisB.x, axisB.y, axisB.z };

    componentObj["useSwingLimits"] = useSwingLimits;
    componentObj["swingSpan1"] = swingSpan1;
    componentObj["swingSpan2"] = swingSpan2;

    componentObj["useTwistLimits"] = useTwistLimits;
    componentObj["twistSpan"] = twistSpan;

    componentObj["limitSoftness"] = limitSoftness;
    componentObj["limitBias"] = limitBias;
    componentObj["limitRelaxation"] = limitRelaxation;
}

void ComponentConeConstraint::Deserialize(const nlohmann::json& componentObj)
{
    ComponentConstraint::Deserialize(componentObj);

    if (componentObj.contains("axisA"))
    {
        auto& axis = componentObj["axisA"];
        axisA = glm::vec3(axis[0], axis[1], axis[2]);
    }

    if (componentObj.contains("axisB"))
    {
        auto& axis = componentObj["axisB"];
        axisB = glm::vec3(axis[0], axis[1], axis[2]);
    }

    if (componentObj.contains("useSwingLimits"))
        useSwingLimits = componentObj["useSwingLimits"].get<bool>();

    if (componentObj.contains("swingSpan1"))
        swingSpan1 = componentObj["swingSpan1"].get<float>();

    if (componentObj.contains("swingSpan2"))
        swingSpan2 = componentObj["swingSpan2"].get<float>();

    if (componentObj.contains("useTwistLimits"))
        useTwistLimits = componentObj["useTwistLimits"].get<bool>();

    if (componentObj.contains("twistSpan"))
        twistSpan = componentObj["twistSpan"].get<float>();

    if (componentObj.contains("limitSoftness"))
        limitSoftness = componentObj["limitSoftness"].get<float>();

    if (componentObj.contains("limitBias"))
        limitBias = componentObj["limitBias"].get<float>();

    if (componentObj.contains("limitRelaxation"))
        limitRelaxation = componentObj["limitRelaxation"].get<float>();
}