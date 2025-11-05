#pragma once

#include "Component.h"
#include "Camera.h"

class MeshData;

class ComponentMesh : public Component
{
public:
	ComponentMesh(GameObject* gameObject);
	virtual ~ComponentMesh();

	void Update();
	void Draw(Camera* camera);
	void DrawVertexNormals(Camera* camera, float length = 0.3f);
	void DrawFaceNormals(Camera* camera, float length = 0.5f);

public:
	int meshIndex = -1;
	bool showVertexNormals = false;
	bool showFaceNormals = false;
};