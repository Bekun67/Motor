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

    float velocity = movementSpeed * time->GetDeltaTime();

    glm::vec3 position = transform->GetPosition();
    glm::vec3 forward = camera->GetFront();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    // WASD movement
    if (keys[SDL_SCANCODE_W])
        position += forward * velocity;
    if (keys[SDL_SCANCODE_S])
        position -= forward * velocity;
    if (keys[SDL_SCANCODE_A])
        position -= right * velocity;
    if (keys[SDL_SCANCODE_D])
        position += right * velocity;

    // Space/Ctrl for vertical movement
    if (keys[SDL_SCANCODE_SPACE])
        position.y += velocity;
    if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL])
        position.y -= velocity;

    transform->SetPosition(position);
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

    // Set position slightly in front of camera
    glm::vec3 spawnPos = camera->GetPosition() + camera->GetFront() * (sphereSize * 2.0f);
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
    }

    // Mark octree for rebuild
    Application::GetInstance().scene->MarkOctreeForRebuild();

    LOG_DEBUG("[FirstPersonController] Fired sphere '%s' with force %.2f",
        sphere->GetName().c_str(), shootForce);
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