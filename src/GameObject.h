#pragma once

#include "imgui.h"
#include <vector>
#include <string>

#include "Component.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"

enum class ComponentType;
class Component;


class GameObject
{

public:
	GameObject();
	GameObject(GameObject* parent);
	~GameObject();

	Component* AddComponent(Component* component);
	Component* GetComponent(ComponentType type);

	
	GameObject* parent;
	std::string name;

	ComponentTransform* transform;
	ComponentMesh* mesh;
	ComponentTexture* texture;

	std::vector<Component*> components;
	std::vector<GameObject*> children;
};