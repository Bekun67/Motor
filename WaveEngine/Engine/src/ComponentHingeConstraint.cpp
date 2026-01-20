#include "ComponentHingeConstraint.h"
#include "GameObject.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Transform.h"
#include "Log.h"
#include <glm/gtc/quaternion.hpp>

ComponentHingeConstraint::ComponentHingeConstraint(GameObject* owner)
    : ComponentConstraint(owner, ConstraintType::HINGE),
    axisA(0.0f, 1.0f, 0.0f),
    axisB(0.0f, 1.0f, 0.0f),
    useLimits(false),
    lowLimit(-glm::pi<float>()),
    highLimit(glm::pi<float>()),
    useMotor(false),
    motorVelocity(0.0f),
    motorMaxImpulse(1.0f)
{
    name = "Hinge Constraint";
}

void ComponentHingeConstraint::CreateConstraint()
{
    if (constraint != nullptr)
    {
        DestroyConstraint();
    }

    Application::PlayState playState = Application::GetInstance().GetPlayState();
    if (playState == Application::PlayState::EDITING)
    {
        LOG_DEBUG("[ComponentHingeConstraint] Skipping constraint creation in EDITING mode");
        return;
    }

    ComponentRigidBody* rbA = GetRigidBody(owner);
    if (!rbA)
    {
        LOG_DEBUG("[ComponentHingeConstraint] Owner '%s' has no RigidBody component", owner->GetName().c_str());
        return;
    }

    if (!rbA->IsActive())
    {
        LOG_DEBUG("[ComponentHingeConstraint] Owner '%s' RigidBody is not active", owner->GetName().c_str());
        return;
    }

    btRigidBody* bodyA = rbA->GetBulletRigidBody();
    if (!bodyA)
    {
        LOG_DEBUG("[ComponentHingeConstraint] Owner '%s' has no Bullet RigidBody", owner->GetName().c_str());
        return;
    }

    btVector3 inertiaA = bodyA->getInvInertiaDiagLocal();

    if (inertiaA.length2() < 0.0001f && bodyA->getMass() > 0.0f)
    {
        LOG_DEBUG("[ComponentHingeConstraint] BodyA has invalid inertia, cannot create constraint");
        return;
    }

    Transform* transformA = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transformA)
    {
        LOG_DEBUG("[ComponentHingeConstraint] Owner has no Transform!");
        return;
    }


    // Create transforms for the constraint frames
    btTransform frameInA;
    frameInA.setIdentity();

    // Set pivot point
    frameInA.setOrigin(btVector3(anchorPointA.x, anchorPointA.y, anchorPointA.z));

    // Create a rotation matrix that aligns Z-axis with our desired hinge axis
    btVector3 hingeAxis(axisA.x, axisA.y, axisA.z);
    hingeAxis.normalize();

    // Create orthonormal basis
    btVector3 tangent, binormal;
    btPlaneSpace1(hingeAxis, tangent, binormal);

    // Build rotation matrix
    btMatrix3x3 rotationMatrix(
        tangent.x(), binormal.x(), hingeAxis.x(),
        tangent.y(), binormal.y(), hingeAxis.y(),
        tangent.z(), binormal.z(), hingeAxis.z()
    );

    frameInA.setBasis(rotationMatrix);

    btHingeConstraint* hingeConstraint = nullptr;

    if (connectedBody)
    {
        ComponentRigidBody* rbB = GetRigidBody(connectedBody);
        if (!rbB)
        {
            LOG_DEBUG("[ComponentHingeConstraint] Connected body '%s' has no RigidBody component", connectedBody->GetName().c_str());
            return;
        }

        if (!rbB->IsActive())
        {
            LOG_DEBUG("[ComponentHingeConstraint] Connected body '%s' RigidBody is not active", connectedBody->GetName().c_str());
            return;
        }

        btRigidBody* bodyB = rbB->GetBulletRigidBody();
        if (!bodyB)
        {
            LOG_DEBUG("[ComponentHingeConstraint] Connected body '%s' has no Bullet RigidBody", connectedBody->GetName().c_str());
            return;
        }

        btVector3 inertiaB = bodyB->getInvInertiaDiagLocal();

        if (inertiaB.length2() < 0.0001f && bodyB->getMass() > 0.0f)
        {
            LOG_DEBUG("[ComponentHingeConstraint] BodyB has invalid inertia, cannot create constraint");
            return;
        }

        btTransform frameInB;
        frameInB.setIdentity();

        frameInB.setOrigin(btVector3(anchorPointB.x, anchorPointB.y, anchorPointB.z));

        btVector3 hingeAxisB(axisB.x, axisB.y, axisB.z);
        hingeAxisB.normalize();

        btVector3 tangentB, binormalB;
        btPlaneSpace1(hingeAxisB, tangentB, binormalB);

        btMatrix3x3 rotationMatrixB(
            tangentB.x(), binormalB.x(), hingeAxisB.x(),
            tangentB.y(), binormalB.y(), hingeAxisB.y(),
            tangentB.z(), binormalB.z(), hingeAxisB.z()
        );

        frameInB.setBasis(rotationMatrixB);

        hingeConstraint = new btHingeConstraint(
            *bodyA, *bodyB,
            frameInA, frameInB
        );

        LOG_DEBUG("[ComponentHingeConstraint] Created hinge between '%s' and '%s'",
            owner->GetName().c_str(),
            connectedBody->GetName().c_str());
    }
    else
    {
        // Create hinge attached to world
        hingeConstraint = new btHingeConstraint(
            *bodyA,
            frameInA,
            true  // useReferenceFrameA
        );

        LOG_DEBUG("[ComponentHingeConstraint] Created hinge for '%s' (attached to world)",
            owner->GetName().c_str());
    }

    if (hingeConstraint)
    {
        // Set limits
        if (useLimits)
        {
            hingeConstraint->setLimit(lowLimit, highLimit);
        }
        else
        {
            hingeConstraint->setLimit(1.0f, -1.0f); // Disable limits
        }

        // Set motor
        if (useMotor)
        {
            hingeConstraint->enableMotor(true);
            hingeConstraint->setMaxMotorImpulse(motorMaxImpulse);
            hingeConstraint->enableAngularMotor(true, motorVelocity, motorMaxImpulse);
        }

        // Set breaking threshold
        if (breakingThreshold > 0.0f)
        {
            hingeConstraint->setBreakingImpulseThreshold(breakingThreshold);
        }

        // Enable/disable
        hingeConstraint->setEnabled(constraintEnabled);

        // Add to physics world
        ModulePhysics* physics = Application::GetInstance().physics.get();
        if (physics && physics->GetDynamicsWorld())
        {
            physics->GetDynamicsWorld()->addConstraint(hingeConstraint, true);
        }

        constraint = hingeConstraint;
    }
}
void ComponentHingeConstraint::UpdateConstraint()
{
    if (!constraint) return;

    btHingeConstraint* hingeConstraint = static_cast<btHingeConstraint*>(constraint);

    // Update limits
    if (useLimits)
    {
        hingeConstraint->setLimit(lowLimit, highLimit);
    }
    else
    {
        hingeConstraint->setLimit(1.0f, -1.0f); // Disable limits
    }

    // Update motor
    hingeConstraint->enableMotor(useMotor);
    if (useMotor)
    {
        hingeConstraint->setMotorTarget(motorVelocity, 1.0f);
        hingeConstraint->setMaxMotorImpulse(motorMaxImpulse);
    }
}

void ComponentHingeConstraint::SetAxisA(const glm::vec3& axis)
{
    axisA = glm::normalize(axis);
    needsRebuild = true;
}

void ComponentHingeConstraint::SetAxisB(const glm::vec3& axis)
{
    axisB = glm::normalize(axis);
    needsRebuild = true;
}

void ComponentHingeConstraint::SetUseLimits(bool use)
{
    useLimits = use;
    UpdateConstraint();
}

void ComponentHingeConstraint::SetLimits(float low, float high)
{
    lowLimit = low;
    highLimit = high;
    UpdateConstraint();
}

void ComponentHingeConstraint::SetUseMotor(bool use)
{
    useMotor = use;
    UpdateConstraint();
}

void ComponentHingeConstraint::SetMotorVelocity(float velocity)
{
    motorVelocity = velocity;
    UpdateConstraint();
}

void ComponentHingeConstraint::SetMotorMaxImpulse(float impulse)
{
    motorMaxImpulse = impulse;
    UpdateConstraint();
}

float ComponentHingeConstraint::GetCurrentAngle() const
{
    if (!constraint) return 0.0f;

    btHingeConstraint* hingeConstraint = static_cast<btHingeConstraint*>(constraint);
    return hingeConstraint->getHingeAngle();
}

void ComponentHingeConstraint::OnEditor()
{
    // This will be called from InspectorWindow
}

void ComponentHingeConstraint::Serialize(nlohmann::json& componentObj) const
{
    ComponentConstraint::Serialize(componentObj);

    componentObj["axisA"] = { axisA.x, axisA.y, axisA.z };
    componentObj["axisB"] = { axisB.x, axisB.y, axisB.z };

    componentObj["useLimits"] = useLimits;
    componentObj["lowLimit"] = lowLimit;
    componentObj["highLimit"] = highLimit;

    componentObj["useMotor"] = useMotor;
    componentObj["motorVelocity"] = motorVelocity;
    componentObj["motorMaxImpulse"] = motorMaxImpulse;
}

void ComponentHingeConstraint::Deserialize(const nlohmann::json& componentObj)
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

    if (componentObj.contains("useLimits"))
        useLimits = componentObj["useLimits"].get<bool>();

    if (componentObj.contains("lowLimit"))
        lowLimit = componentObj["lowLimit"].get<float>();

    if (componentObj.contains("highLimit"))
        highLimit = componentObj["highLimit"].get<float>();

    if (componentObj.contains("useMotor"))
        useMotor = componentObj["useMotor"].get<bool>();

    if (componentObj.contains("motorVelocity"))
        motorVelocity = componentObj["motorVelocity"].get<float>();

    if (componentObj.contains("motorMaxImpulse"))
        motorMaxImpulse = componentObj["motorMaxImpulse"].get<float>();
}