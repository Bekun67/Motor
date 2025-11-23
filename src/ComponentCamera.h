#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class ProjectionType
{
    PERSPECTIVE,
    ORTHOGRAPHIC
};

enum class FrustumIntersection
{
    OUT,
    INTERSECT,
    IN
};

struct Plane
{
    glm::vec3 normal;
    float distance;

    Plane() : normal(0, 0, 0), distance(0) {}

    void SetFromPointNormal(const glm::vec3& point, const glm::vec3& n)
    {
        normal = glm::normalize(n);
        distance = glm::dot(normal, point);
    }

    float SignedDistance(const glm::vec3& point) const
    {
        return glm::dot(normal, point) + distance;
    }

    bool IsOnPositiveSide(const glm::vec3& point) const
    {
        return SignedDistance(point) >= 0.0f;
    }
};

struct Frustum
{
    Plane planes[6];

    void ExtractFromMatrix(const glm::mat4& viewProjection);
    FrustumIntersection ContainsAABB(const glm::vec3& minPoint, const glm::vec3& maxPoint) const;
};

class ComponentCamera : public Component
{
public:
    ComponentCamera(GameObject* gameObject);
    virtual ~ComponentCamera();

    void Update();

    // Matrix getters
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewProjectionMatrix() const;

    // Frustum
    void UpdateFrustum();
    const Frustum& GetFrustum() const { return frustum; }

    // Camera properties
    void SetFOV(float fovDegrees);
    void SetAspectRatio(float aspect);
    void SetNearPlane(float near);
    void SetFarPlane(float far);
    void SetProjectionType(ProjectionType type);

    float GetFOV() const { return fov; }
    float GetAspectRatio() const { return aspectRatio; }
    float GetNearPlane() const { return nearPlane; }
    float GetFarPlane() const { return farPlane; }
    ProjectionType GetProjectionType() const { return projectionType; }

    // Background
    glm::vec3 backgroundColor;

    bool debugRaycastEnabled = false;

private:
    // Projection parameters
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    ProjectionType projectionType;

    // Frustum culling
    Frustum frustum;

    // For orthographic
    float orthographicSize;
};