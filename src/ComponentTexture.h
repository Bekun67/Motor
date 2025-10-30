#pragma once

#include "Component.h"
#include <assimp/scene.h>

class ComponentTexture : public Component
{
public:
	ComponentTexture(GameObject* gameObject);
	virtual ~ComponentTexture();

	void Update();

	void LoadTexture(const aiScene* scene, const aiNode* node, unsigned int i);


	//void AddTexture(Texture* texture);

public:
	//Texture* materialTexture;
	//GLuint textureId;


};