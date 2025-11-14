#pragma once
#include "Module.h"
#include "Ray.h"
#include <vector>

class GameObject;
class Camera;

struct RayHit
{
    GameObject* gameObject = nullptr;
    float distance = FLT_MAX;
    glm::vec3 hitPoint;
    bool hit = false;

    RayHit() = default;
    RayHit(GameObject* go, float dist, const glm::vec3& point)
        : gameObject(go), distance(dist), hitPoint(point), hit(true) {
    }
};

class ModuleMousePicking : public Module
{
public:
    ModuleMousePicking();
    ~ModuleMousePicking();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    Ray CreateRayFromMouse(Camera* camera, float mouseX, float mouseY, int screenWidth, int screenHeight);

    RayHit CastRay(const Ray& ray, const std::vector<GameObject*>& gameObjects);

    RayHit CastRayAgainstGameObject(const Ray& ray, GameObject* gameObject);

    bool TestRayAgainstMesh(const Ray& rayLocalSpace, int meshIndex, float& closestDistance, glm::vec3& closestHitPoint);

private:
    bool enableDebugDraw = false;
};