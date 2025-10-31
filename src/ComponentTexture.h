#pragma once

#include "Component.h"
#include <assimp/scene.h>
#include "LoadFBX.h"

class ComponentTexture : public Component
{
public:
	ComponentTexture(GameObject* gameObject);
	virtual ~ComponentTexture();

	void Update();

	void LoadTexture(const aiScene* scene, const aiNode* node, unsigned int i);


	

public:

	TextureData* texturedata;

};