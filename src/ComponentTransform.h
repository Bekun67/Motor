#pragma once

#include "Component.h"
#include <assimp/scene.h>
#include <glm/glm.hpp>


class ComponentTransform : public Component
{
public:
	ComponentTransform(GameObject* gameObject);
	virtual ~ComponentTransform();

	void Update() override;
	bool Decompose(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);

	// Serialization
	PropertyMap Serialize() const override;
	void Deserialize(const PropertyMap& props) override;

	aiVector3D scaling, translation;
	aiQuaternion rotation;

	glm::vec3 position;
	glm::vec3 scale;
	glm::mat4 rotationMatrix;

	float radius;
};