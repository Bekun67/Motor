#pragma once
#include <glm/glm.hpp>

struct Ray;

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    bool IntersectRay(const Ray& ray, float& tMin, float& tMax) const;

    glm::vec3 GetCenter() const
    {
        return (min + max) * 0.5f;
    }

    glm::vec3 GetSize() const
    {
        return max - min;
    }
};

struct WorldAABB
{
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 center;
    glm::vec3 size;
};