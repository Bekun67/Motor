#include "ComponentSliderConstraint.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Log.h"

ComponentSliderConstraint::ComponentSliderConstraint(GameObject* owner)
    : ComponentConstraint(owner, ConstraintType::SLIDER),
    axisA(1.0f, 0.0f, 0.0f),
    axisB(1.0f, 0.0f, 0.0f),
    useLinearLimits(false),
    lowerLinearLimit(-1.0f),
    upperLinearLimit(1.0f),
    useAngularLimits(false),
    lowerAngularLimit(-glm::pi<float>()),
    upperAngularLimit(glm::pi<float>())
{
    name = "Slider Constraint";
}

void ComponentSliderConstraint::CreateConstraint()
{
    if (constraint != nullptr)
    {
        DestroyConstraint();
    }

    Application::PlayState playState = Application::GetInstance().GetPlayState();
    if (playState == Application::PlayState::EDITING)
    {
        LOG_DEBUG("[ComponentSliderConstraint] Skipping constraint creation in EDITING mode");
        return;
    }

    ComponentRigidBody* rbA = GetRigidBody(owner);
    if (!rbA)
    {
        LOG_DEBUG("[ComponentSliderConstraint] Owner '%s' has no RigidBody component", owner->GetName().c_str());
        return;
    }

    if (!rbA->IsActive())
    {
        LOG_DEBUG("[ComponentSliderConstraint] Owner '%s' RigidBody is not active", owner->GetName().c_str());
        return;
    }

    btRigidBody* bodyA = rbA->GetBulletRigidBody();
    if (!bodyA)
    {
        LOG_DEBUG("[ComponentSliderConstraint] Owner '%s' has no Bullet RigidBody", owner->GetName().c_str());
        return;
    }

    btVector3 inertiaA = bodyA->getInvInertiaDiagLocal();

    if (inertiaA.length2() < 0.0001f && bodyA->getMass() > 0.0f)
    {
        LOG_DEBUG("[ComponentSliderConstraint] BodyA has invalid inertia, cannot create constraint");
        return;
    }

    Transform* transformA = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transformA)
    {
        LOG_DEBUG("[ComponentSliderConstraint] Owner has no Transform!");
        return;
    }


    // Create frame transform for slider
    btTransform frameInA;
    frameInA.setIdentity();
    frameInA.setOrigin(btVector3(anchorPointA.x, anchorPointA.y, anchorPointA.z));

    // Slider axis (X-axis in local frame by default)
    btVector3 sliderAxis(axisA.x, axisA.y, axisA.z);
    sliderAxis.normalize();

    // Create orthonormal basis with slider axis as X
    btVector3 yAxis, zAxis;
    btPlaneSpace1(sliderAxis, yAxis, zAxis);

    btMatrix3x3 rotationMatrix(
        sliderAxis.x(), yAxis.x(), zAxis.x(),
        sliderAxis.y(), yAxis.y(), zAxis.y(),
        sliderAxis.z(), yAxis.z(), zAxis.z()
    );

    frameInA.setBasis(rotationMatrix);

    btSliderConstraint* sliderConstraint = nullptr;

    if (connectedBody)
    {
        ComponentRigidBody* rbB = GetRigidBody(connectedBody);
        if (!rbB)
        {
            LOG_DEBUG("[ComponentSliderConstraint] Connected body '%s' has no RigidBody component", connectedBody->GetName().c_str());
            return;
        }

        if (!rbB->IsActive())
        {
            LOG_DEBUG("[ComponentSliderConstraint] Connected body '%s' RigidBody is not active", connectedBody->GetName().c_str());
            return;
        }

        btRigidBody* bodyB = rbB->GetBulletRigidBody();
        if (!bodyB)
        {
            LOG_DEBUG("[ComponentSliderConstraint] Connected body '%s' has no Bullet RigidBody", connectedBody->GetName().c_str());
            return;
        }

        btVector3 inertiaB = bodyB->getInvInertiaDiagLocal();

        if (inertiaB.length2() < 0.0001f && bodyB->getMass() > 0.0f)
        {
            LOG_DEBUG("[ComponentSliderConstraint] BodyB has invalid inertia, cannot create constraint");
            return;
        }

        btTransform frameInB;
        frameInB.setIdentity();
        frameInB.setOrigin(btVector3(anchorPointB.x, anchorPointB.y, anchorPointB.z));

        btVector3 sliderAxisB(axisB.x, axisB.y, axisB.z);
        sliderAxisB.normalize();

        btVector3 yAxisB, zAxisB;
        btPlaneSpace1(sliderAxisB, yAxisB, zAxisB);

        btMatrix3x3 rotationMatrixB(
            sliderAxisB.x(), yAxisB.x(), zAxisB.x(),
            sliderAxisB.y(), yAxisB.y(), zAxisB.y(),
            sliderAxisB.z(), yAxisB.z(), zAxisB.z()
        );

        frameInB.setBasis(rotationMatrixB);

        sliderConstraint = new btSliderConstraint(
            *bodyA, *bodyB,
            frameInA, frameInB,
            true
        );

        LOG_DEBUG("[ComponentSliderConstraint] Created slider between '%s' and '%s'",
            owner->GetName().c_str(),
            connectedBody->GetName().c_str());
    }
    else
    {
        sliderConstraint = new btSliderConstraint(
            *bodyA,
            frameInA,
            true  // useLinearReferenceFrameA
        );

        LOG_DEBUG("[ComponentSliderConstraint] Created slider for '%s' (attached to world)",
            owner->GetName().c_str());
    }

    if (sliderConstraint)
    {
        // Set linear limits
        if (useLinearLimits)
        {
            sliderConstraint->setLowerLinLimit(lowerLinearLimit);
            sliderConstraint->setUpperLinLimit(upperLinearLimit);
        }
        else
        {
            sliderConstraint->setLowerLinLimit(1.0f);
            sliderConstraint->setUpperLinLimit(-1.0f);
        }

        // Set angular limits
        if (useAngularLimits)
        {
            sliderConstraint->setLowerAngLimit(lowerAngularLimit);
            sliderConstraint->setUpperAngLimit(upperAngularLimit);
        }
        else
        {
            sliderConstraint->setLowerAngLimit(1.0f);
            sliderConstraint->setUpperAngLimit(-1.0f);
        }

        // Set breaking threshold
        if (breakingThreshold > 0.0f)
        {
            sliderConstraint->setBreakingImpulseThreshold(breakingThreshold);
        }

        // Enable/disable
        sliderConstraint->setEnabled(constraintEnabled);

        // Add to physics world
        ModulePhysics* physics = Application::GetInstance().physics.get();
        if (physics && physics->GetDynamicsWorld())
        {
            physics->GetDynamicsWorld()->addConstraint(sliderConstraint, true);
        }

        constraint = sliderConstraint;
    }
}

void ComponentSliderConstraint::UpdateConstraint()
{
    if (!constraint) return;

    btSliderConstraint* sliderConstraint = static_cast<btSliderConstraint*>(constraint);

    // Update linear limits
    if (useLinearLimits)
    {
        sliderConstraint->setLowerLinLimit(lowerLinearLimit);
        sliderConstraint->setUpperLinLimit(upperLinearLimit);
    }
    else
    {
        sliderConstraint->setLowerLinLimit(1.0f);
        sliderConstraint->setUpperLinLimit(-1.0f);
    }

    // Update angular limits
    if (useAngularLimits)
    {
        sliderConstraint->setLowerAngLimit(lowerAngularLimit);
        sliderConstraint->setUpperAngLimit(upperAngularLimit);
    }
    else
    {
        sliderConstraint->setLowerAngLimit(1.0f);
        sliderConstraint->setUpperAngLimit(-1.0f);
    }
}

void ComponentSliderConstraint::SetAxisA(const glm::vec3& axis)
{
    axisA = glm::normalize(axis);
    needsRebuild = true;
}

void ComponentSliderConstraint::SetAxisB(const glm::vec3& axis)
{
    axisB = glm::normalize(axis);
    needsRebuild = true;
}

void ComponentSliderConstraint::SetUseLinearLimits(bool use)
{
    useLinearLimits = use;
    UpdateConstraint();
}

void ComponentSliderConstraint::SetLinearLimits(float lower, float upper)
{
    lowerLinearLimit = lower;
    upperLinearLimit = upper;
    UpdateConstraint();
}

void ComponentSliderConstraint::SetUseAngularLimits(bool use)
{
    useAngularLimits = use;
    UpdateConstraint();
}

void ComponentSliderConstraint::SetAngularLimits(float lower, float upper)
{
    lowerAngularLimit = lower;
    upperAngularLimit = upper;
    UpdateConstraint();
}

float ComponentSliderConstraint::GetCurrentLinearPosition() const
{
    if (!constraint) return 0.0f;

    btSliderConstraint* sliderConstraint = static_cast<btSliderConstraint*>(constraint);
    return sliderConstraint->getLinearPos();
}

float ComponentSliderConstraint::GetCurrentAngularPosition() const
{
    if (!constraint) return 0.0f;

    btSliderConstraint* sliderConstraint = static_cast<btSliderConstraint*>(constraint);
    return sliderConstraint->getAngularPos();
}

void ComponentSliderConstraint::OnEditor()
{
    // This will be called from InspectorWindow
}

void ComponentSliderConstraint::Serialize(nlohmann::json& componentObj) const
{
    ComponentConstraint::Serialize(componentObj);

    componentObj["axisA"] = { axisA.x, axisA.y, axisA.z };
    componentObj["axisB"] = { axisB.x, axisB.y, axisB.z };

    componentObj["useLinearLimits"] = useLinearLimits;
    componentObj["lowerLinearLimit"] = lowerLinearLimit;
    componentObj["upperLinearLimit"] = upperLinearLimit;

    componentObj["useAngularLimits"] = useAngularLimits;
    componentObj["lowerAngularLimit"] = lowerAngularLimit;
    componentObj["upperAngularLimit"] = upperAngularLimit;
}

void ComponentSliderConstraint::Deserialize(const nlohmann::json& componentObj)
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

    if (componentObj.contains("useLinearLimits"))
        useLinearLimits = componentObj["useLinearLimits"].get<bool>();

    if (componentObj.contains("lowerLinearLimit"))
        lowerLinearLimit = componentObj["lowerLinearLimit"].get<float>();

    if (componentObj.contains("upperLinearLimit"))
        upperLinearLimit = componentObj["upperLinearLimit"].get<float>();

    if (componentObj.contains("useAngularLimits"))
        useAngularLimits = componentObj["useAngularLimits"].get<bool>();

    if (componentObj.contains("lowerAngularLimit"))
        lowerAngularLimit = componentObj["lowerAngularLimit"].get<float>();

    if (componentObj.contains("upperAngularLimit"))
        upperAngularLimit = componentObj["upperAngularLimit"].get<float>();
}