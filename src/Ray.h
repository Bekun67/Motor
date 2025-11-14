#pragma once
#include <glm/glm.hpp>

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;

    Ray() : origin(0.0f), direction(0.0f, 0.0f, -1.0f) {}
    Ray(const glm::vec3& origin, const glm::vec3& direction)
        : origin(origin), direction(glm::normalize(direction)) {
    }

    glm::vec3 GetPoint(float t) const
    {
        return origin + direction * t;
    }
};

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    bool IntersectRay(const Ray& ray, float& tMin, float& tMax) const
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

    glm::vec3 GetCenter() const
    {
        return (min + max) * 0.5f;
    }

    glm::vec3 GetSize() const
    {
        return max - min;
    }
};


struct Triangle
{
    glm::vec3 v0, v1, v2;

    Triangle() : v0(0.0f), v1(0.0f), v2(0.0f) {}
    Triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
        : v0(v0), v1(v1), v2(v2) {
    }

    bool IntersectRay(const Ray& ray, float& t, glm::vec3& hitPoint) const
    {
        const float EPSILON = 0.0000001f;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);

        if (a > -EPSILON && a < EPSILON)
            return false; 

        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f)
            return false;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);

        if (v < 0.0f || u + v > 1.0f)
            return false;

        t = f * glm::dot(edge2, q);

        if (t > EPSILON)
        {
            hitPoint = ray.GetPoint(t);
            return true;
        }

        return false;
    }
};