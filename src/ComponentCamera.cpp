#include "ComponentCamera.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

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

void Frustum::ExtractFromMatrix(const glm::mat4& m)
{
    //extract all 6 planes from aabb
    planes[0].normal.x = m[0][3] + m[0][0];
    planes[0].normal.y = m[1][3] + m[1][0];
    planes[0].normal.z = m[2][3] + m[2][0];
    planes[0].distance = m[3][3] + m[3][0];

    planes[1].normal.x = m[0][3] - m[0][0];
    planes[1].normal.y = m[1][3] - m[1][0];
    planes[1].normal.z = m[2][3] - m[2][0];
    planes[1].distance = m[3][3] - m[3][0];

    planes[2].normal.x = m[0][3] + m[0][1];
    planes[2].normal.y = m[1][3] + m[1][1];
    planes[2].normal.z = m[2][3] + m[2][1];
    planes[2].distance = m[3][3] + m[3][1];

    planes[3].normal.x = m[0][3] - m[0][1];
    planes[3].normal.y = m[1][3] - m[1][1];
    planes[3].normal.z = m[2][3] - m[2][1];
    planes[3].distance = m[3][3] - m[3][1];

    planes[4].normal.x = m[0][3] + m[0][2];
    planes[4].normal.y = m[1][3] + m[1][2];
    planes[4].normal.z = m[2][3] + m[2][2];
    planes[4].distance = m[3][3] + m[3][2];

    planes[5].normal.x = m[0][3] - m[0][2];
    planes[5].normal.y = m[1][3] - m[1][2];
    planes[5].normal.z = m[2][3] - m[2][2];
    planes[5].distance = m[3][3] - m[3][2];

    //normalize all planes
    for (int i = 0; i < 6; i++)
    {
        float length = glm::length(planes[i].normal);
        if (length > 0.0001f)
        {
            float invLength = 1.0f / length;
            planes[i].normal *= invLength;
            planes[i].distance *= invLength;
        }
    }
}

FrustumIntersection Frustum::ContainsAABB(const glm::vec3& minPoint, const glm::vec3& maxPoint) const
{
    int totalIn = 0;

    //test against each plane (6 of them)
    for (int p = 0; p < 6; ++p)
    {
        //furthest vertex
        glm::vec3 positiveVertex;
        positiveVertex.x = (planes[p].normal.x >= 0.0f) ? maxPoint.x : minPoint.x;
        positiveVertex.y = (planes[p].normal.y >= 0.0f) ? maxPoint.y : minPoint.y;
        positiveVertex.z = (planes[p].normal.z >= 0.0f) ? maxPoint.z : minPoint.z;

        //closests vertex
        glm::vec3 negativeVertex;
        negativeVertex.x = (planes[p].normal.x >= 0.0f) ? minPoint.x : maxPoint.x;
        negativeVertex.y = (planes[p].normal.y >= 0.0f) ? minPoint.y : maxPoint.y;
        negativeVertex.z = (planes[p].normal.z >= 0.0f) ? minPoint.z : maxPoint.z;

        //if the positive vertex is outside then the entire box is out and we end the method
        float distPositive = glm::dot(planes[p].normal, positiveVertex) + planes[p].distance;
        if (distPositive < 0.0f) return FrustumIntersection::OUT;

        //if the negative vertex is inside then the entire box is in
        float distNegative = glm::dot(planes[p].normal, negativeVertex) + planes[p].distance;
        if (distNegative >= 0.0f) totalIn++;
    }

    //if all 6 planes are in we are IN
    if (totalIn == 6) return FrustumIntersection::IN;

    //otherwise we are INTERESECT
    return FrustumIntersection::INTERSECT;
}