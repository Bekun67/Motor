#pragma once

#include "Component.h"
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>


class ComponentTransform : public Component
{
public:
	ComponentTransform(GameObject* gameObject);
	virtual ~ComponentTransform();

	void Update();
	bool Decompose(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);

	aiVector3D scaling, translation;
	aiQuaternion rotation;

	glm::vec3 position;
	glm::vec3 scale;
	glm::mat4 rotationMatrix;
};