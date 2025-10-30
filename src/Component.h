#pragma once

class GameObject;

enum class ComponentType
{
	NONE,
	TRANSFORM,
	TEXTURE,
	MESH,
};

class Component
{
public:
	Component(GameObject* owner, ComponentType type)
	{
		active = true;
		this->Parent = owner;
	}

	~Component() {}

	bool active;
	ComponentType type;
	GameObject* Parent;
};