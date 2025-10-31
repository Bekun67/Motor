#pragma once

#include "Component.h"
#include <assimp/scene.h>
#include <glm/glm.hpp>


class ComponentTransform : public Component
{
public:
	ComponentTransform(GameObject* gameObject);
	virtual ~ComponentTransform();

	void Update();

	aiVector3D scaling, translation;
	aiQuaternion rotation;

	void SetFromNode(const aiNode* node);

};