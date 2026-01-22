#include "ComponentFirstPersonController.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentCamera.h"
#include "ComponentRigidBody.h"
#include "ComponentCollider.h"
#include "ComponentMaterial.h"
#include "ComponentMesh.h"
#include "Primitives.h"   
#include "Application.h"
#include "Input.h"
#include "Time.h"
#include "ModuleScene.h"
#include "Log.h"
#include <SDL3/SDL.h>
#include <glm/gtc/quaternion.hpp>

ComponentFirstPersonController::ComponentFirstPersonController(GameObject* owner)
    : Component(owner, ComponentType::FIRSTPERSON),
    movementSpeed(5.0f),
    shootForce(20.0f),
    sphereSize(0.5f),
    mouseSensitivity(0.2f),
    yaw(-90.0f),
    pitch(0.0f),
    firstMouse(true),
    lastMouseX(0.0f),
    lastMouseY(0.0f),
    isRightMousePressed(false)
{
    name = "FirstPersonController";
    LOG_CONSOLE("[FirstPersonController] Created on '%s'", owner->GetName().c_str());
}

ComponentFirstPersonController::~ComponentFirstPersonController()
{
    LOG_CONSOLE("[FirstPersonController] Destroying on '%s'", owner->GetName().c_str());
}

void ComponentFirstPersonController::Enable()
{
    LOG_CONSOLE("[FirstPersonController] Enabled on '%s'", owner->GetName().c_str());
    firstMouse = true;
}

void ComponentFirstPersonController::Disable()
{
    LOG_CONSOLE("[FirstPersonController] Disabled on '%s'", owner->GetName().c_str());
}

void ComponentFirstPersonController::Update()
{
    if (!IsActive()) return;

    // Only update when in play mode
    Application::PlayState playState = Application::GetInstance().GetPlayState();
    if (playState != Application::PlayState::PLAYING) return;

    HandleMovement();
    HandleMouseLook();

    // Shoot sphere on left mouse click
    Input* input = Application::GetInstance().input.get();
    if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
    {
        ShootSphere();
    }
}

void ComponentFirstPersonController::HandleMovement()
{
    Input* input = Application::GetInstance().input.get();
    const bool* keys = SDL_GetKeyboardState(NULL);
    Time* time = Application::GetInstance().time.get();

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    ComponentCamera* camera = static_cast<ComponentCamera*>(owner->GetComponent(ComponentType::CAMERA));
    if (!camera) return;

    ComponentRigidBody* rigidBody = static_cast<ComponentRigidBody*>(
        owner->GetComponent(ComponentType::RIGIDBODY)
        );

    float deltaTime = time->GetDeltaTime();
    if (deltaTime <= 0.0f) return;

    // Calculate directions based on camera orientation
    glm::vec3 forward = camera->GetFront();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    // Forward direction projected onto the horizontal plane (for WASD)
    glm::vec3 horizontalForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
    glm::vec3 horizontalRight = glm::normalize(glm::vec3(right.x, 0.0f, right.z));

    if (rigidBody && rigidBody->IsActive())
    {
		// with rigidbody

        // Make the RigidBody kinematic so it doesn't fall due to gravity
        if (!rigidBody->IsKinematic())
        {
            rigidBody->SetKinematic(true);
            LOG_DEBUG("[FirstPersonController] Set RigidBody to kinematic mode");
        }

        glm::vec3 moveDirection(0.0f);

        // WASD movement on the horizontal plane
        if (keys[SDL_SCANCODE_W])
            moveDirection += horizontalForward;
        if (keys[SDL_SCANCODE_S])
            moveDirection -= horizontalForward;
        if (keys[SDL_SCANCODE_A])
            moveDirection -= horizontalRight;
        if (keys[SDL_SCANCODE_D])
            moveDirection += horizontalRight;

        // Normalize if moving diagonally
        if (glm::length(moveDirection) > 0.01f)
        {
            moveDirection = glm::normalize(moveDirection);
        }

        // Vertical movement (up/down)
        if (keys[SDL_SCANCODE_SPACE])
            moveDirection.y += 1.0f;
        if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL])
            moveDirection.y -= 1.0f;

        // Apply movement directly to position (kinematic mode)
        if (glm::length(moveDirection) > 0.01f)
        {
            glm::vec3 position = transform->GetPosition();
            glm::vec3 movement = moveDirection * movementSpeed * deltaTime;
            position += movement;
            transform->SetPosition(position);

            // Sync with physics
            rigidBody->SyncTransformToPhysics();
        }

        // Cancel any residual linear velocity
        rigidBody->SetVelocity(glm::vec3(0.0f));

        // Cancel angular velocity
        btRigidBody* btBody = rigidBody->GetBulletRigidBody();
        if (btBody)
        {
            btBody->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
        }
    }
    else
    {
		// without rigidbody

        float velocity = movementSpeed * deltaTime;
        glm::vec3 position = transform->GetPosition();

        // WASD movement on the horizontal plane
        if (keys[SDL_SCANCODE_W])
            position += horizontalForward * velocity;
        if (keys[SDL_SCANCODE_S])
            position -= horizontalForward * velocity;
        if (keys[SDL_SCANCODE_A])
            position -= horizontalRight * velocity;
        if (keys[SDL_SCANCODE_D])
            position += horizontalRight * velocity;

        // Space/Ctrl for vertical movement
        if (keys[SDL_SCANCODE_SPACE])
            position.y += velocity;
        if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL])
            position.y -= velocity;

        transform->SetPosition(position);
    }
}

void ComponentFirstPersonController::HandleMouseLook()
{
    Input* input = Application::GetInstance().input.get();

    // Check if right mouse button is pressed
    bool rightMouseDown = (input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN ||
        input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_REPEAT);

    if (!rightMouseDown)
    {
        firstMouse = true;
        isRightMousePressed = false;
        return;
    }

    isRightMousePressed = true;

    float mouseX = static_cast<float>(input->GetMouseX());
    float mouseY = static_cast<float>(input->GetMouseY());

    if (firstMouse)
    {
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        firstMouse = false;
        return;
    }

    float xOffset = mouseX - lastMouseX;
    float yOffset = lastMouseY - mouseY; // Reversed since y-coordinates go from bottom to top
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    // Constrain pitch
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Update camera orientation
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    glm::vec3 front = glm::normalize(direction);
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    // Create rotation matrix
    glm::mat4 rotationMatrix(1.0f);
    rotationMatrix[0] = glm::vec4(right, 0.0f);
    rotationMatrix[1] = glm::vec4(up, 0.0f);
    rotationMatrix[2] = glm::vec4(-front, 0.0f);

    // Convert to quaternion and apply to transform
    glm::quat rotation = glm::quat_cast(rotationMatrix);
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        transform->SetRotationQuat(rotation);

        // Sync with physics if it has a RigidBody
        ComponentRigidBody* rigidBody = static_cast<ComponentRigidBody*>(
            owner->GetComponent(ComponentType::RIGIDBODY)
            );
        if (rigidBody && rigidBody->IsActive())
        {
            rigidBody->SyncTransformToPhysics();
        }
    }
}

void ComponentFirstPersonController::ShootSphere()
{
    ComponentCamera* camera = static_cast<ComponentCamera*>(owner->GetComponent(ComponentType::CAMERA));
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));

    if (!camera || !transform) return;

    // Create a completely NEW and INDEPENDENT sphere GameObject
    std::string sphereName = "Projectile_" + std::to_string(SDL_GetTicks());
    GameObject* sphere = new GameObject(sphereName);
    sphere->isPrimitive = true;

    // Add to scene root (important for proper lifecycle management)
    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (root)
    {
        root->AddChild(sphere);
    }

    // Spawn farther away to avoid collision with the camera
    float spawnDistance = sphereSize * 3.0f;

    // Add an extra offset if the camera has a collider
    ComponentCollider* cameraCollider = static_cast<ComponentCollider*>(
        owner->GetComponent(ComponentType::COLLIDER)
        );
    if (cameraCollider && cameraCollider->IsActive())
    {
        // If the camera has a collider, spawn even farther away
        spawnDistance += 1.5f;
    }

    glm::vec3 spawnPos = camera->GetPosition() + camera->GetFront() * spawnDistance;

    Transform* sphereTransform = static_cast<Transform*>(sphere->GetComponent(ComponentType::TRANSFORM));
    if (sphereTransform)
    {
        sphereTransform->SetPosition(spawnPos);
        sphereTransform->SetScale(glm::vec3(sphereSize));
    }

    // Add mesh component with sphere primitive
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(sphere->CreateComponent(ComponentType::MESH));
    if (meshComp)
    {
        // Create sphere primitive mesh
        Mesh sphereMesh = Primitives::CreateSphere(0.5f, 16, 16);
        meshComp->SetMesh(sphereMesh);
    }

    // Add material (checkerboard by default)
    ComponentMaterial* material = static_cast<ComponentMaterial*>(sphere->CreateComponent(ComponentType::MATERIAL));
    if (material)
    {
        material->CreateCheckerboardTexture();
    }

    // Add sphere collider BEFORE rigid body
    ComponentCollider* collider = sphere->CreateCollider(ColliderType::SPHERE);
    if (collider)
    {
        collider->SetSphereRadius(0.5f); // Scale handles the actual size
        collider->SetFriction(0.3f);
        collider->SetRestitution(0.6f);
    }

    // Add rigid body
    ComponentRigidBody* rb = static_cast<ComponentRigidBody*>(sphere->CreateComponent(ComponentType::RIGIDBODY));
    if (rb)
    {
        rb->SetMass(1.0f);

        // Apply impulse in camera direction
        glm::vec3 shootDirection = camera->GetFront();
        rb->ApplyImpulse(shootDirection * shootForce);

        LOG_DEBUG("[FirstPersonController] Fired sphere at (%.2f, %.2f, %.2f) with impulse (%.2f, %.2f, %.2f)",
            spawnPos.x, spawnPos.y, spawnPos.z,
            shootDirection.x * shootForce,
            shootDirection.y * shootForce,
            shootDirection.z * shootForce);
    }

    // Mark octree for rebuild
    Application::GetInstance().scene->MarkOctreeForRebuild();

    LOG_DEBUG("[FirstPersonController] Fired sphere '%s' with force %.2f at distance %.2f",
        sphere->GetName().c_str(), shootForce, spawnDistance);
}

void ComponentFirstPersonController::OnEditor()
{
    // This will be called from InspectorWindow
}

void ComponentFirstPersonController::Serialize(nlohmann::json& componentObj) const
{
    componentObj["movementSpeed"] = movementSpeed;
    componentObj["shootForce"] = shootForce;
    componentObj["sphereSize"] = sphereSize;
    componentObj["mouseSensitivity"] = mouseSensitivity;
}

void ComponentFirstPersonController::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("movementSpeed"))
        movementSpeed = componentObj["movementSpeed"].get<float>();

    if (componentObj.contains("shootForce"))
        shootForce = componentObj["shootForce"].get<float>();

    if (componentObj.contains("sphereSize"))
        sphereSize = componentObj["sphereSize"].get<float>();

    if (componentObj.contains("mouseSensitivity"))
        mouseSensitivity = componentObj["mouseSensitivity"].get<float>();
}