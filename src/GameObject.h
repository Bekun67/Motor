#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include "Component.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "Ray.h" 

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

	void UpdateAABB();

	AABB GetWorldAABB() const;

	GameObject* parent;
	std::string name;

	ComponentTransform* transform;
	ComponentMesh* mesh;
	ComponentTexture* texture;

	std::vector<Component*> components;
	std::vector<GameObject*> children;

	AABB localAABB;
	bool hasAABB = false;
};