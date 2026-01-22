#include "ComponentVehicleController.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "Input.h"
#include "Time.h"
#include "Log.h"
#include <SDL3/SDL.h>
#include <glm/gtx/quaternion.hpp>

ComponentVehicleController::ComponentVehicleController(GameObject* owner)
    : Component(owner, ComponentType::VEHICLE),
    acceleration(10.0f),
    maxSpeed(15.0f),
    turnSpeed(90.0f),
    brakeForce(20.0f),
    drag(0.98f),
    forwardAxis(0.0f, 0.0f, 1.0f), 
    currentVelocity(0.0f),
    currentSpeed(0.0f),
    forwardInput(0.0f),
    turnInput(0.0f),
    isBraking(false)
{
    name = "VehicleController";
    LOG_CONSOLE("[VehicleController] Created on '%s'", owner->GetName().c_str());
}

ComponentVehicleController::~ComponentVehicleController()
{
    LOG_CONSOLE("[VehicleController] Destroying on '%s'", owner->GetName().c_str());
}

void ComponentVehicleController::Enable()
{
    LOG_CONSOLE("[VehicleController] Enabled on '%s'", owner->GetName().c_str());
    currentVelocity = glm::vec3(0.0f);
    currentSpeed = 0.0f;
}

void ComponentVehicleController::Disable()
{
    LOG_CONSOLE("[VehicleController] Disabled on '%s'", owner->GetName().c_str());
}

void ComponentVehicleController::Update()
{
    if (!IsActive()) return;

    // Only update when in play mode
    Application::PlayState playState = Application::GetInstance().GetPlayState();
    if (playState != Application::PlayState::PLAYING) return;

    // Safety check
    if (!owner || owner->IsBeingDestroyed() || owner->IsMarkedForDeletion()) return;

    HandleInput();
    ApplyMovement();
}

void ComponentVehicleController::HandleInput()
{
    const bool* keys = SDL_GetKeyboardState(NULL);

    // Forward/Backward input
    forwardInput = 0.0f;
    if (keys[SDL_SCANCODE_UP])
        forwardInput = 1.0f;
    if (keys[SDL_SCANCODE_DOWN])
        forwardInput = -1.0f;

    // Turn input
    turnInput = 0.0f;
    if (keys[SDL_SCANCODE_LEFT])
        turnInput = 1.0f;
    if (keys[SDL_SCANCODE_RIGHT])
        turnInput = -1.0f;

    // Brake
    isBraking = keys[SDL_SCANCODE_LSHIFT];
}

void ComponentVehicleController::ApplyMovement()
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    ComponentRigidBody* rigidBody = static_cast<ComponentRigidBody*>(
        owner->GetComponent(ComponentType::RIGIDBODY)
        );

    Time* time = Application::GetInstance().time.get();
    float deltaTime = time->GetDeltaTime();

    if (deltaTime <= 0.0f) return;

    // Get current rotation
    glm::quat currentRotation = transform->GetRotationQuat();

    if (rigidBody && rigidBody->IsActive())
    {
		// with RigidBody

        // Get current velocity from RigidBody
        glm::vec3 currentVel = rigidBody->GetVelocity();

        // Calculate speed in the horizontal plane (ignore Y)
        glm::vec3 horizontalVel = glm::vec3(currentVel.x, 0.0f, currentVel.z);
        currentSpeed = glm::length(horizontalVel);

		// apply rotation
        if (glm::abs(turnInput) > 0.01f)
        {
            // Calculate turn amount
            float turnAmount = turnInput * turnSpeed * deltaTime;

            if (currentSpeed > 0.1f) 
            {
                float speedFactor = glm::clamp(currentSpeed / (maxSpeed * 0.3f), 0.3f, 1.5f);
                turnAmount *= speedFactor;
            }
            else
            {
                turnAmount *= 0.5f;
            }

            // Apply rotation around Y axis (up)
            glm::quat turnRotation = glm::angleAxis(glm::radians(turnAmount), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat newRotation = turnRotation * currentRotation;

			// update transform rotation
            transform->SetRotationQuat(newRotation);
            rigidBody->SyncTransformToPhysics();

            currentRotation = newRotation;
        }

		// calculate rotated forward and right vectors
        glm::vec3 forward = glm::rotate(currentRotation, forwardAxis);
        glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));

        if (currentSpeed > 0.1f)
        {
            glm::vec3 forwardVel = forward * glm::dot(horizontalVel, forward);
            glm::vec3 rightVel = right * glm::dot(horizontalVel, right);

			// reduce lateral velocity
            float lateralFriction = 0.85f; 
            glm::vec3 correctedVel = forwardVel + rightVel * lateralFriction;

			// Mantain current vertical velocity
            correctedVel.y = currentVel.y;

            rigidBody->SetVelocity(correctedVel);
        }

        // Calculate target force
        glm::vec3 targetForce(0.0f);

        if (isBraking)
        {
            // Apply braking force opposite to movement
            if (currentSpeed > 0.1f)
            {
                glm::vec3 brakeDirection = -glm::normalize(horizontalVel);
                targetForce = brakeDirection * brakeForce;
            }
        }
        else if (glm::abs(forwardInput) > 0.01f)
        {
            targetForce = forward * forwardInput * acceleration;

            if (currentSpeed > 0.1f)
            {
                glm::vec3 velDirection = glm::normalize(horizontalVel);
                float alignment = glm::dot(velDirection, forward);

                if ((forwardInput > 0.0f && alignment < -0.3f) ||
                    (forwardInput < 0.0f && alignment > 0.3f))
                {
                    targetForce += -velDirection * brakeForce * 0.5f;
                }
            }
        }
        else
        {
            // Apply drag when no input
            if (currentSpeed > 0.1f)
            {
                glm::vec3 dragDirection = -glm::normalize(horizontalVel);
                targetForce = dragDirection * (currentSpeed * (1.0f - drag) * 10.0f);
            }
        }

        // Apply force to RigidBody
        if (glm::length(targetForce) > 0.01f)
        {
            rigidBody->ApplyForce(targetForce);
        }
    }
    else
    {
		// Without RigidBody
        glm::vec3 position = transform->GetPosition();
        glm::vec3 forward = glm::rotate(currentRotation, forwardAxis);

        // Apply rotation 
        if (glm::abs(turnInput) > 0.01f)
        {
            float turnAmount = turnInput * turnSpeed * deltaTime;

            // Only turn effectively when moving
            if (glm::abs(currentSpeed) > 0.5f)
            {
                float speedFactor = glm::min(glm::abs(currentSpeed) / maxSpeed, 1.0f);
                turnAmount *= speedFactor;
            }
            else
            {
                turnAmount *= 0.3f;
            }

            glm::quat turnRotation = glm::angleAxis(glm::radians(turnAmount), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat newRotation = turnRotation * currentRotation;
            transform->SetRotationQuat(newRotation);

            forward = glm::rotate(newRotation, forwardAxis);
        }

        // Calculate velocity
        if (isBraking)
        {
            // Decelerate
            if (currentSpeed > 0.0f)
            {
                currentSpeed -= brakeForce * deltaTime;
                if (currentSpeed < 0.0f) currentSpeed = 0.0f;
            }
            else if (currentSpeed < 0.0f)
            {
                currentSpeed += brakeForce * deltaTime;
                if (currentSpeed > 0.0f) currentSpeed = 0.0f;
            }
        }
        else if (glm::abs(forwardInput) > 0.01f)
        {
            // Accelerate
            currentSpeed += forwardInput * acceleration * deltaTime;

            // Clamp speed
            if (currentSpeed > 0.0f)
                currentSpeed = glm::min(currentSpeed, maxSpeed);
            else
                currentSpeed = glm::max(currentSpeed, -maxSpeed * 0.5f);
        }
        else
        {
            // Apply drag
            if (currentSpeed > 0.0f)
            {
                currentSpeed -= (1.0f - drag) * currentSpeed * 10.0f * deltaTime;
                if (currentSpeed < 0.1f) currentSpeed = 0.0f;
            }
            else if (currentSpeed < 0.0f)
            {
                currentSpeed += (1.0f - drag) * glm::abs(currentSpeed) * 10.0f * deltaTime;
                if (currentSpeed > -0.1f) currentSpeed = 0.0f;
            }
        }

        // Apply movement
        if (glm::abs(currentSpeed) > 0.01f)
        {
            glm::vec3 movement = forward * currentSpeed * deltaTime;
            position += movement;
            transform->SetPosition(position);
        }
    }
}

void ComponentVehicleController::OnEditor()
{
    // Called from InspectorWindow
}

void ComponentVehicleController::Serialize(nlohmann::json& componentObj) const
{
    componentObj["acceleration"] = acceleration;
    componentObj["maxSpeed"] = maxSpeed;
    componentObj["turnSpeed"] = turnSpeed;
    componentObj["brakeForce"] = brakeForce;
    componentObj["drag"] = drag;
    componentObj["forwardAxis"] = { forwardAxis.x, forwardAxis.y, forwardAxis.z };  // <-- NUEVO
}

void ComponentVehicleController::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("acceleration"))
        acceleration = componentObj["acceleration"].get<float>();

    if (componentObj.contains("maxSpeed"))
        maxSpeed = componentObj["maxSpeed"].get<float>();

    if (componentObj.contains("turnSpeed"))
        turnSpeed = componentObj["turnSpeed"].get<float>();

    if (componentObj.contains("brakeForce"))
        brakeForce = componentObj["brakeForce"].get<float>();

    if (componentObj.contains("drag"))
        drag = componentObj["drag"].get<float>();

    if (componentObj.contains("forwardAxis") && componentObj["forwardAxis"].is_array())
    {
        auto& axis = componentObj["forwardAxis"];
        if (axis.size() == 3)
        {
            forwardAxis = glm::vec3(axis[0], axis[1], axis[2]);
            forwardAxis = glm::normalize(forwardAxis);
        }
    }
}