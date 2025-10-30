#pragma once

#include "Component.h"

#include "LoadFBX.h"
#include "Camera.h"

class MeshData;

class ComponentMesh : public Component
{
public:
	ComponentMesh(GameObject* gameObject);
	virtual ~ComponentMesh();

	void Update();
	void Draw(Camera* camera);

public:
	MeshData* mesh;
	bool drawOutline = false;


};