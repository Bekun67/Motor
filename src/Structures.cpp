#include "Structures.h"
#include "Ray.h"

bool AABB::IntersectRay(const Ray& ray, float& tMin, float& tMax) const
{
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t0s = (min - ray.origin) * invDir;
    glm::vec3 t1s = (max - ray.origin) * invDir;

    glm::vec3 tsmaller = glm::min(t0s, t1s);
    glm::vec3 tbigger = glm::max(t0s, t1s);

    tMin = glm::max(tsmaller.x, glm::max(tsmaller.y, tsmaller.z));
    tMax = glm::min(tbigger.x, glm::min(tbigger.y, tbigger.z));

    return tMin <= tMax && tMax >= 0.0f;
}