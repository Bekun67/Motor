#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include "Component.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "Ray.h" 
#include "UUID.h"

//forward declarations
class Component;
class ComponentTransform;
class ComponentMesh;
class ComponentTexture;
class ComponentCamera;
enum class ComponentType;

class GameObject
{
public:
	GameObject();
	GameObject(GameObject* parent);
	~GameObject();

	Component* AddComponent(Component* component);
	Component* GetComponent(ComponentType type);

	void UpdateAABB();

	AABB GetWorldAABB() const;
	UUID GetUUID() const { return m_UUID; }
	void SetUUID(UUID uuid) { m_UUID = uuid; }

	UUID GetParentUUID() const { return m_ParentUUID; }
	void SetParentUUID(UUID uuid) { m_ParentUUID = uuid; }

	// Hierarchy management
	void SetParent(GameObject* newParent);
	void RemoveChild(GameObject* child);
	void AddChild(GameObject* child);
	int GetChildIndex() const;
	void MoveUp();
	void MoveDown();

	// Helper to check if this is an empty GameObject (no mesh)
	bool IsEmpty() const { return mesh == nullptr || mesh->meshIndex < 0; }

	// Get all descendants (children, grandchildren, etc.)
	void GetAllDescendants(std::vector<GameObject*>& descendants);

	// Check if this GameObject is a descendant of another
	bool IsDescendantOf(GameObject* potentialAncestor) const;

	GameObject* parent;
	std::string name;
	bool active = true;

	ComponentTransform* transform;
	ComponentMesh* mesh;
	ComponentTexture* texture;
	ComponentCamera* camera;

	std::vector<Component*> components;
	std::vector<GameObject*> children;

	AABB localAABB;
	bool hasAABB = false;
	float distanceToCamera = 0.0f;
	std::string meshPath;
	int meshIndexInFBX = 0;

	// Frustum culling state
	bool isVisibleInFrustum = true;
	bool culledLastFrame = false;

private:
	UUID m_UUID;
	UUID m_ParentUUID;
};