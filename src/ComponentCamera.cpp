#include "ComponentCamera.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

ComponentCamera::ComponentCamera(GameObject* gameObject)
    : Component(gameObject, ComponentType::CAMERA),
    fov(60.0f),
    aspectRatio(16.0f / 9.0f),
    nearPlane(1.0f), 
    farPlane(1000.0f),
    projectionType(ProjectionType::PERSPECTIVE),
    backgroundColor(0.1f, 0.1f, 0.12f),
    orthographicSize(10.0f)
{
}

ComponentCamera::~ComponentCamera()
{
}

void ComponentCamera::Update()
{
    UpdateFrustum();
}

glm::mat4 ComponentCamera::GetViewMatrix() const
{
    if (!gameObject || !gameObject->transform)
        return glm::mat4(1.0f);

    ComponentTransform* transform = gameObject->transform;

    glm::vec3 position(
        transform->translation.x,
        transform->translation.y,
        transform->translation.z
    );

    glm::quat rotation(
        transform->rotation.w,
        transform->rotation.x,
        transform->rotation.y,
        transform->rotation.z
    );

    glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

    return glm::lookAt(position, position + forward, up);
}

glm::mat4 ComponentCamera::GetProjectionMatrix() const
{
    if (projectionType == ProjectionType::PERSPECTIVE)
    {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }
    else
    {
        float halfHeight = orthographicSize;
        float halfWidth = halfHeight * aspectRatio;
        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
    }
}

glm::mat4 ComponentCamera::GetViewProjectionMatrix() const
{
    return GetProjectionMatrix() * GetViewMatrix();
}

void ComponentCamera::UpdateFrustum()
{
    glm::mat4 viewProj = GetViewProjectionMatrix();
    frustum.ExtractFromMatrix(viewProj);
}

void ComponentCamera::SetFOV(float fovDegrees)
{
    fov = fovDegrees;
}

void ComponentCamera::SetAspectRatio(float aspect)
{
    aspectRatio = aspect;
}

void ComponentCamera::SetNearPlane(float near)
{
    nearPlane = near;
}

void ComponentCamera::SetFarPlane(float far)
{
    farPlane = far;
}

void ComponentCamera::SetProjectionType(ProjectionType type)
{
    projectionType = type;
}

void Frustum::ExtractFromMatrix(const glm::mat4& vp)
{
    // Left plane
    planes[0].normal.x = vp[0][3] + vp[0][0];
    planes[0].normal.y = vp[1][3] + vp[1][0];
    planes[0].normal.z = vp[2][3] + vp[2][0];
    planes[0].distance = vp[3][3] + vp[3][0];

    // Right plane
    planes[1].normal.x = vp[0][3] - vp[0][0];
    planes[1].normal.y = vp[1][3] - vp[1][0];
    planes[1].normal.z = vp[2][3] - vp[2][0];
    planes[1].distance = vp[3][3] - vp[3][0];

    // Bottom plane
    planes[2].normal.x = vp[0][3] + vp[0][1];
    planes[2].normal.y = vp[1][3] + vp[1][1];
    planes[2].normal.z = vp[2][3] + vp[2][1];
    planes[2].distance = vp[3][3] + vp[3][1];

    // Top plane
    planes[3].normal.x = vp[0][3] - vp[0][1];
    planes[3].normal.y = vp[1][3] - vp[1][1];
    planes[3].normal.z = vp[2][3] - vp[2][1];
    planes[3].distance = vp[3][3] - vp[3][1];

    // Near plane
    planes[4].normal.x = vp[0][3] + vp[0][2];
    planes[4].normal.y = vp[1][3] + vp[1][2];
    planes[4].normal.z = vp[2][3] + vp[2][2];
    planes[4].distance = vp[3][3] + vp[3][2];

    // Far plane
    planes[5].normal.x = vp[0][3] - vp[0][2];
    planes[5].normal.y = vp[1][3] - vp[1][2];
    planes[5].normal.z = vp[2][3] - vp[2][2];
    planes[5].distance = vp[3][3] - vp[3][2];

    // Normalize all planes
    for (int i = 0; i < 6; i++)
    {
        float length = glm::length(planes[i].normal);
        planes[i].normal /= length;
        planes[i].distance /= length;
    }
}

FrustumIntersection Frustum::ContainsAABB(const glm::vec3& minPoint, const glm::vec3& maxPoint) const
{
    glm::vec3 corners[8] = {
        glm::vec3(minPoint.x, minPoint.y, minPoint.z),
        glm::vec3(maxPoint.x, minPoint.y, minPoint.z),
        glm::vec3(minPoint.x, maxPoint.y, minPoint.z),
        glm::vec3(maxPoint.x, maxPoint.y, minPoint.z),
        glm::vec3(minPoint.x, minPoint.y, maxPoint.z),
        glm::vec3(maxPoint.x, minPoint.y, maxPoint.z),
        glm::vec3(minPoint.x, maxPoint.y, maxPoint.z),
        glm::vec3(maxPoint.x, maxPoint.y, maxPoint.z)
    };

    int totalIn = 0;

    for (int p = 0; p < 6; ++p)
    {
        int inCount = 8;
        int ptIn = 1;

        for (int i = 0; i < 8; ++i)
        {
            if (!planes[p].IsOnPositiveSide(corners[i]))
            {
                ptIn = 0;
                --inCount;
            }
        }

        if (inCount == 0) return FrustumIntersection::OUT;
        totalIn += ptIn;
    }

    if (totalIn == 6)
        return FrustumIntersection::IN;

    return FrustumIntersection::INTERSECT;
}