#pragma once
#include "Module.h"
#include "Ray.h"
#include "Quadtree.h"
#include <glm/glm.hpp>
#include <vector>

class GameObject;
class Camera;

struct RaycastHit
{
    GameObject* gameObject = nullptr;
    float distance = 0.0f;
    glm::vec3 hitPoint = glm::vec3(0.0f);
    bool hit = false;
};

class ModuleMousePicking : public Module
{
public:
    ModuleMousePicking();
    ~ModuleMousePicking();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    // Create a ray from camera through mouse position
    Ray CreateRayFromMouse(float mouseX, float mouseY, Camera* camera, int screenWidth, int screenHeight);

    // Cast ray against all game objects
    RaycastHit CastRay(const Ray& ray, const std::vector<GameObject*>& gameObjects);

    // Test ray in AABB
    bool RayIntersectsAABB(const Ray& ray, const AABB& aabb, float& tMin, float& tMax);

    // Test ray with triangle
    bool RayIntersectsTriangle(const Ray& ray, const Triangle& triangle, float& distance, glm::vec3& hitPoint);

    // Get all triangles from a game object
    std::vector<Triangle> GetMeshTriangles(GameObject* gameObject);

private:
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
};
