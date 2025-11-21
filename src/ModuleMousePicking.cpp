#include "ModuleMousePicking.h"
#include "Application.h"
#include "Input.h"
#include "OpenGL.h"
#include "Camera.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "LoadFBX.h"
#include "ModuleEditor.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

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
    // Check for left mouse button click
    Input* input = Application::GetInstance().input.get();
    OpenGL* opengl = Application::GetInstance().opengl.get();
    ModuleEditor* editor = Application::GetInstance().editor.get();

    if (!input || !opengl || !editor) return true;

    // Only process picking if left mouse button is clicked
    if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) != KEY_DOWN)
        return true;

    if (editor->editing)
        return true;

    // Get mouse position
    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    // Check if mouse is inside scene viewport
    ImVec2 viewportMin = editor->sceneViewportPos;
    ImVec2 viewportMax = ImVec2(
        viewportMin.x + editor->sceneViewportSize.x,
        viewportMin.y + editor->sceneViewportSize.y
    );

    if (mouseX < viewportMin.x || mouseX > viewportMax.x ||
        mouseY < viewportMin.y || mouseY > viewportMax.y)
    {
        // Mouse outside scene viewport
        return true;
    }

    float relativeX = mouseX - viewportMin.x;
    float relativeY = mouseY - viewportMin.y;

    // creates a ray from the mouse
    Ray ray = CreateRayFromMouse(
        relativeX,
        relativeY,
        &opengl->camera,
        (int)editor->sceneViewportSize.x,
        (int)editor->sceneViewportSize.y
    );

    // Check if the ray hits something
    RaycastHit hit = CastRay(ray, opengl->gameObjects);

    if (hit.hit && hit.gameObject)
    {
        // If the ray detects something, it selects it
        opengl->selectedGameObject = hit.gameObject;
        editor->selectedGameObject = hit.gameObject;

        LOG("Picked GameObject: " + hit.gameObject->name +
            " at distance: " + std::to_string(hit.distance));
    }
    else
    {
        // if the ray doesn't detect anything, it deselects
        opengl->selectedGameObject = nullptr;
        editor->selectedGameObject = nullptr;
        LOG("No GameObject picked - deselecting");
    }

    return true;
}

bool ModuleMousePicking::CleanUp()
{
    LOG("Cleaning up ModuleMousePicking");
    return true;
}

Ray ModuleMousePicking::CreateRayFromMouse(float mouseX, float mouseY, Camera* camera, int screenWidth, int screenHeight)
{
    // Give the mouse its starting coordinates
    float normalizedX = (2.0f * mouseX) / screenWidth - 1.0f;
    float normalizedY = 1.0f - (2.0f * mouseY) / screenHeight;

    glm::vec4 rayClip = glm::vec4(normalizedX, normalizedY, -1.0f, 1.0f);

    glm::mat4 invProjection = glm::inverse(camera->GetProjectionMatrix());
    glm::vec4 rayEye = invProjection * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::mat4 invView = glm::inverse(camera->GetViewMatrix());
    glm::vec4 rayWorld4 = invView * rayEye;
    glm::vec3 rayWorld = glm::vec3(rayWorld4.x, rayWorld4.y, rayWorld4.z);
    rayWorld = glm::normalize(rayWorld);

    // create the final ray
    Ray ray;
    ray.origin = camera->GetPosition();
    ray.direction = rayWorld;

    return ray;
}

RaycastHit ModuleMousePicking::CastRay(const Ray& ray, const std::vector<GameObject*>& gameObjects)
{
    RaycastHit closestHit;
    closestHit.distance = FLT_MAX;

    // List to store objects that the ray hits
    std::vector<std::pair<GameObject*, float>> aabbHits;

    for (GameObject* go : gameObjects)
    {
        if (!go || !go->mesh || go->mesh->meshIndex < 0 || go->mesh->meshIndex >= (int)g_Meshes.size())
            continue;

        MeshData& meshData = g_Meshes[go->mesh->meshIndex];
        ComponentTransform* transform = go->transform;

        // transformation matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(
            transform->translation.x,
            transform->translation.y,
            transform->translation.z
        ));

        aiQuaternion aiRot = transform->rotation;
        aiMatrix3x3 aiRotMatrix = aiRot.GetMatrix();

        glm::mat4 rotationMatrix = glm::mat4(
            aiRotMatrix.a1, aiRotMatrix.a2, aiRotMatrix.a3, 0.0f,
            aiRotMatrix.b1, aiRotMatrix.b2, aiRotMatrix.b3, 0.0f,
            aiRotMatrix.c1, aiRotMatrix.c2, aiRotMatrix.c3, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
        model *= rotationMatrix;

        model = glm::scale(model, glm::vec3(
            transform->scaling.x,
            transform->scaling.y,
            transform->scaling.z
        ));

        // Create AABB
        glm::vec3 corners[8] = {
            glm::vec3(meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMin.z),
            glm::vec3(meshData.aabbMax.x, meshData.aabbMin.y, meshData.aabbMin.z),
            glm::vec3(meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMin.z),
            glm::vec3(meshData.aabbMin.x, meshData.aabbMax.y, meshData.aabbMin.z),
            glm::vec3(meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMax.z),
            glm::vec3(meshData.aabbMax.x, meshData.aabbMin.y, meshData.aabbMax.z),
            glm::vec3(meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMax.z),
            glm::vec3(meshData.aabbMin.x, meshData.aabbMax.y, meshData.aabbMax.z)
        };

        glm::vec3 worldMin(FLT_MAX);
        glm::vec3 worldMax(-FLT_MAX);

        for (int i = 0; i < 8; ++i)
        {
            glm::vec4 worldCorner = model * glm::vec4(corners[i], 1.0f);
            glm::vec3 corner3 = glm::vec3(worldCorner);

            worldMin = glm::min(worldMin, corner3);
            worldMax = glm::max(worldMax, corner3);
        }

        AABB worldAABB(worldMin, worldMax);

        float distance;
        if (RayIntersectsAABB(ray, worldAABB, distance))
        {
            aabbHits.push_back({ go, distance });
        }
    }

    std::sort(aabbHits.begin(), aabbHits.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    for (const auto& [go, aabbDistance] : aabbHits)
    {
        std::vector<Triangle> triangles = GetMeshTriangles(go);

        for (const Triangle& triangle : triangles)
        {
            float distance;
            glm::vec3 hitPoint;

            if (RayIntersectsTriangle(ray, triangle, distance, hitPoint))
            {
                if (distance < closestHit.distance)
                {
                    closestHit.hit = true;
                    closestHit.gameObject = go;
                    closestHit.distance = distance;
                    closestHit.hitPoint = hitPoint;
                }
            }
        }

        // If we find a hit, we can stop 
        if (closestHit.hit)
            break;
    }

    return closestHit;
}

bool ModuleMousePicking::RayIntersectsAABB(const Ray& ray, const AABB& aabb, float& distance)
{
    float tMin, tMax;
    return aabb.IntersectRay(ray, tMin, tMax);
}

bool ModuleMousePicking::RayIntersectsTriangle(const Ray& ray, const Triangle& triangle, float& distance, glm::vec3& hitPoint)
{
    return triangle.IntersectRay(ray, distance, hitPoint);
}

std::vector<Triangle> ModuleMousePicking::GetMeshTriangles(GameObject* gameObject)
{
    std::vector<Triangle> triangles;

    if (!gameObject || !gameObject->mesh || gameObject->mesh->meshIndex < 0)
        return triangles;

    int meshIndex = gameObject->mesh->meshIndex;
    if (meshIndex >= (int)g_Meshes.size())
        return triangles;

    MeshData& meshData = g_Meshes[meshIndex];
    ComponentTransform* transform = gameObject->transform;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        transform->translation.x,
        transform->translation.y,
        transform->translation.z
    ));

    aiQuaternion aiRot = transform->rotation;
    aiMatrix3x3 aiRotMatrix = aiRot.GetMatrix();

    glm::mat4 rotationMatrix = glm::mat4(
        aiRotMatrix.a1, aiRotMatrix.a2, aiRotMatrix.a3, 0.0f,
        aiRotMatrix.b1, aiRotMatrix.b2, aiRotMatrix.b3, 0.0f,
        aiRotMatrix.c1, aiRotMatrix.c2, aiRotMatrix.c3, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    model *= rotationMatrix;

    model = glm::scale(model, glm::vec3(
        transform->scaling.x,
        transform->scaling.y,
        transform->scaling.z
    ));

    glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
    GLint bufferSize;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

    int vertexSize = 8;
    int numVertices = bufferSize / (vertexSize * sizeof(float));

    std::vector<float> vertexData(bufferSize / sizeof(float));
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertexData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.EBO);
    GLint eboSize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);

    std::vector<unsigned int> indices(eboSize / sizeof(unsigned int));
    glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboSize, indices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

        glm::vec4 worldV0 = model * glm::vec4(v0, 1.0f);
        glm::vec4 worldV1 = model * glm::vec4(v1, 1.0f);
        glm::vec4 worldV2 = model * glm::vec4(v2, 1.0f);

        Triangle triangle;
        triangle.v0 = glm::vec3(worldV0);
        triangle.v1 = glm::vec3(worldV1);
        triangle.v2 = glm::vec3(worldV2);

        triangles.push_back(triangle);
    }

    return triangles;
}
