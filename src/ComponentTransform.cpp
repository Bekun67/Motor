#include "ComponentTransform.h"
#include "GameObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>

ComponentTransform::ComponentTransform(GameObject* gameObject)
    : Component(gameObject, ComponentType::TRANSFORM) 
{
    //predeterminated values
    scaling = aiVector3D(1.0f, 1.0f, 1.0f);
    translation = aiVector3D(0.0f, 0.0f, 0.0f);
    rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
}

ComponentTransform::~ComponentTransform()
{

}

void ComponentTransform::Update()
{
}

bool ComponentTransform::Decompose(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale)
{
	glm::mat4 localMatrix(transform);

	translation = glm::vec3(localMatrix[3]);

	glm::vec3 row[3];

	for (int i = 0; i < 3; i++)
	{
		row[i] = glm::vec3(localMatrix[i]);
	}

	scale.x = glm::length(row[0]);
	scale.y = glm::length(row[1]);
	scale.z = glm::length(row[2]);

	if (scale.x != 0) row[0] /= scale.x;
	if (scale.y != 0) row[1] /= scale.y;
	if (scale.z != 0) row[2] /= scale.z;

	glm::mat3 rotationMatrix(row[0], row[1], row[2]);

	rotation = glm::quat_cast(rotationMatrix);

	return true;
}