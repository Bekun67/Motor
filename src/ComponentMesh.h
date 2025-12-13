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
	void DrawOutline(Camera* camera, const glm::vec3& color, float thickness);
	void DrawVertexNormals(Camera* camera, float length = 0.3f);
	void DrawFaceNormals(Camera* camera, float length = 0.5f);
	void DrawAABB(Camera* camera);
	void DrawDebugRay(Camera* camera);

	//method to get the previous aabb struct in the moduleEditor.cpp
	WorldAABB GetWorldAABB() const;

	// Serialization
	PropertyMap Serialize() const override;
	void Deserialize(const PropertyMap& props) override;

public:
	int meshIndex = -1;
	bool showVertexNormals = false;
	bool showFaceNormals = false;
	bool showAABB = false;
};