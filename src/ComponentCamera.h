#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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