#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Component.h"
#include "Structures.h"

class GameObject;
struct Ray;

enum class ProjectionType
{
    PERSPECTIVE,
    ORTHOGRAPHIC
};

struct Plane
{
    glm::vec3 normal;
    float distance;
};

enum class FrustumIntersection
{
    OUT,
    INTERSECT,
    IN
};

struct Frustum
{
    Plane planes[6];

    void ExtractFromMatrix(const glm::mat4& m);
    FrustumIntersection ContainsAABB(const glm::vec3& minPoint, const glm::vec3& maxPoint) const;

    bool Intersects(const AABB& box) const
    {
        for (int p = 0; p < 6; ++p)
        {
            glm::vec3 positiveVertex;
            positiveVertex.x = (planes[p].normal.x >= 0.0f) ? box.max.x : box.min.x;
            positiveVertex.y = (planes[p].normal.y >= 0.0f) ? box.max.y : box.min.y;
            positiveVertex.z = (planes[p].normal.z >= 0.0f) ? box.max.z : box.min.z;

            float dist = glm::dot(planes[p].normal, positiveVertex) + planes[p].distance;
            if (dist < 0.0f) return false;
        }

        return true; 
    }

    bool Intersects(const WorldAABB& worldAABB) const
    {
        AABB box(worldAABB.min, worldAABB.max);
        return Intersects(box);
    }
};

class ComponentCamera : public Component
{
public:
    ComponentCamera(GameObject* gameObject);
    virtual ~ComponentCamera();

    void Update() override;

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewProjectionMatrix() const;

    void UpdateFrustum();

    void SetFOV(float fovDegrees);
    void SetAspectRatio(float aspect);
    void SetNearPlane(float near);
    void SetFarPlane(float far);
    void SetProjectionType(ProjectionType type);

    float GetFOV() const { return fov; }
    float GetAspectRatio() const { return aspectRatio; }
    float GetNearPlane() const { return nearPlane; }
    float GetFarPlane() const { return farPlane; }

    // Get frustum for culling
    const Frustum& GetFrustum() const { return frustum; }

    // Serialization
    PropertyMap Serialize() const override;
    void Deserialize(const PropertyMap& props) override;

public:
    ProjectionType projectionType;
    glm::vec3 backgroundColor;
    float orthographicSize;
    Frustum frustum;
    bool debugRaycastEnabled = false;

private:
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
};