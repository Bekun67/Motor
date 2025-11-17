#include "ModuleMousePicking.h"
#include "Application.h"
#include "OpenGL.h"
#include "Camera.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "LoadFBX.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

ModuleMousePicking::ModuleMousePicking()
{
}

ModuleMousePicking::~ModuleMousePicking()
{
}

bool ModuleMousePicking::Start()
{
    LOG("ModuleMousePicking initialized");
    return true;
}

bool ModuleMousePicking::Update()
{
    // Check for mouse click
    Input* input = Application::GetInstance().input.get();
    if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
    {
        float mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        Window* window = Application::GetInstance().window.get();
        int screenWidth, screenHeight;
        window->GetWindowSize(screenWidth, screenHeight);

        Camera* camera = Application::GetInstance().camera.get();

        Ray ray = CreateRayFromMouse(camera, mouseX, mouseY, screenWidth, screenHeight);

        OpenGL* opengl = Application::GetInstance().opengl.get();
        RayHit hit = CastRay(ray, opengl->gameObjects);

        if (hit.hit)
        {
            opengl->selectedGameObject = hit.gameObject;

            ModuleEditor* editor = Application::GetInstance().editor.get();
            if (editor)
            {
                editor->selectedGameObject = hit.gameObject;
            }

            LOG("Selected: " + hit.gameObject->name + " (distance: " +
                std::to_string(hit.distance) + ")");
        }
        else
        {

            opengl->selectedGameObject = nullptr;
            ModuleEditor* editor = Application::GetInstance().editor.get();
            if (editor)
            {
                editor->selectedGameObject = nullptr;
            }
            LOG("Deselected (no hit)");
        }
    }

    return true;
}

bool ModuleMousePicking::CleanUp()
{
    return true;
}

Ray ModuleMousePicking::CreateRayFromMouse(Camera* camera, float mouseX, float mouseY,
    int screenWidth, int screenHeight)
{

    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;

    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

    glm::mat4 projInverse = glm::inverse(camera->GetProjectionMatrix());
    glm::vec4 rayEye = projInverse * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::mat4 viewInverse = glm::inverse(camera->GetViewMatrix());
    glm::vec3 rayWorld = glm::vec3(viewInverse * rayEye);
    rayWorld = glm::normalize(rayWorld);

    return Ray(camera->GetPosition(), rayWorld);
}

RayHit ModuleMousePicking::CastRay(const Ray& ray, const std::vector<GameObject*>& gameObjects)
{
    RayHit closestHit;
    closestHit.distance = FLT_MAX;

    for (GameObject* go : gameObjects)
    {
        if (go == nullptr || go->mesh == nullptr || go->mesh->meshIndex < 0)
            continue;

        RayHit hit = CastRayAgainstGameObject(ray, go);

        if (hit.hit && hit.distance < closestHit.distance)
        {
            closestHit = hit;
        }
    }

    return closestHit;
}

RayHit ModuleMousePicking::CastRayAgainstGameObject(const Ray& ray, GameObject* gameObject)
{
    RayHit result;

    if (gameObject->mesh->meshIndex < 0 || gameObject->mesh->meshIndex >= (int)g_Meshes.size())
        return result;

    ComponentTransform* transform = gameObject->transform;
    if (transform == nullptr)
        return result;

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(
        transform->translation.x,
        transform->translation.y,
        transform->translation.z
    ));

    glm::quat quat(
        transform->rotation.w,
        transform->rotation.x,
        transform->rotation.y,
        transform->rotation.z
    );
    model *= glm::mat4_cast(quat);

    model = glm::scale(model, glm::vec3(
        transform->scaling.x,
        transform->scaling.y,
        transform->scaling.z
    ));

    glm::mat4 inverseModel = glm::inverse(model);
    glm::vec3 localOrigin = glm::vec3(inverseModel * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDirection = glm::vec3(inverseModel * glm::vec4(ray.direction, 0.0f));
    localDirection = glm::normalize(localDirection);

    Ray localRay(localOrigin, localDirection);

    float closestDistance = FLT_MAX;
    glm::vec3 closestHitPoint;

    if (TestRayAgainstMesh(localRay, gameObject->mesh->meshIndex, closestDistance, closestHitPoint))
    {
        glm::vec3 worldHitPoint = glm::vec3(model * glm::vec4(closestHitPoint, 1.0f));
        float worldDistance = glm::length(worldHitPoint - ray.origin);

        result.hit = true;
        result.gameObject = gameObject;
        result.distance = worldDistance;
        result.hitPoint = worldHitPoint;
    }

    return result;
}

bool ModuleMousePicking::TestRayAgainstMesh(const Ray& rayLocalSpace, int meshIndex,
    float& closestDistance, glm::vec3& closestHitPoint)
{
    if (meshIndex < 0 || meshIndex >= (int)g_Meshes.size())
        return false;

    MeshData& meshData = g_Meshes[meshIndex];

    glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
    GLint vboSize;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vboSize);

    std::vector<float> vertexData(vboSize / sizeof(float));
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, vboSize, vertexData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.EBO);
    GLint eboSize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);

    std::vector<unsigned int> indices(eboSize / sizeof(unsigned int));
    glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboSize, indices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    int vertexSize = 8; 
    closestDistance = FLT_MAX;
    bool hitFound = false;

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        int offset0 = idx0 * vertexSize;
        int offset1 = idx1 * vertexSize;
        int offset2 = idx2 * vertexSize;

        glm::vec3 v0(vertexData[offset0], vertexData[offset0 + 1], vertexData[offset0 + 2]);
        glm::vec3 v1(vertexData[offset1], vertexData[offset1 + 1], vertexData[offset1 + 2]);
        glm::vec3 v2(vertexData[offset2], vertexData[offset2 + 1], vertexData[offset2 + 2]);

        Triangle tri(v0, v1, v2);

        float t;
        glm::vec3 hitPoint;
        if (tri.IntersectRay(rayLocalSpace, t, hitPoint))
        {
            if (t < closestDistance)
            {
                closestDistance = t;
                closestHitPoint = hitPoint;
                hitFound = true;
            }
        }
    }

    return hitFound;
}