#pragma once

#include "Component.h"
#include "Camera.h"

class MeshData;

class ComponentMesh : public Component
{
public:
	ComponentMesh(GameObject* gameObject);
	virtual ~ComponentMesh();

	void Update() override;
	void Draw(Camera* camera);
	void DrawVertexNormals(Camera* camera, float length = 0.3f);
	void DrawFaceNormals(Camera* camera, float length = 0.5f);
	void DrawAABB(Camera* camera);

	// Serialization
	PropertyMap Serialize() const override;
	void Deserialize(const PropertyMap& props) override;

public:
	int meshIndex = -1;
	bool showVertexNormals = false;
	bool showFaceNormals = false;
	bool showAABB = false;
};