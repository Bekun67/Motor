#pragma once

#include "imgui.h"
#include <vector>
#include <string>

#include "Component.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "UUID.h"

//forward declatarions
class Component;
class ComponentTransform;
class ComponentMesh;
class ComponentTexture;
enum class ComponentType;

class GameObject
{

public:
	GameObject();
	GameObject(GameObject* parent);
	~GameObject();

	Component* AddComponent(Component* component);
	Component* GetComponent(ComponentType type);

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

	GameObject* parent;
	std::string name;
	bool active = true;

	ComponentTransform* transform;
	ComponentMesh* mesh;
	ComponentTexture* texture;

	std::vector<Component*> components;
	std::vector<GameObject*> children;

	float distanceToCamera = 0.0f;
	std::string meshPath;
	int meshIndexInFBX = 0;

private:
	UUID m_UUID;
	UUID m_ParentUUID;
};