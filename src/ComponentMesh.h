#pragma once

#include "Component.h"
#include "Camera.h"
#include <glm/glm.hpp>

class MeshData;

//aabb struct for showing in the inspector
struct WorldAABB
{
	glm::vec3 min;
	glm::vec3 max;
	glm::vec3 center;
	glm::vec3 size;
};

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