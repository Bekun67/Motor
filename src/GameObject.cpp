#include "GameObject.h"
#include "Component.h"

GameObject::GameObject()
{
	name = "gameObject";
	parent = nullptr;

	transform = new ComponentTransform(this);
	mesh = new ComponentMesh(this);
	texture = new ComponentTexture(this);
	camera = nullptr;

	isVisibleInFrustum = true;
	culledLastFrame = false;
}


GameObject::GameObject(GameObject* parent)
{
	name = "gameObject";
	this->parent = parent;

	if (parent != nullptr) {
		parent->children.push_back(this);
	}

	transform = new ComponentTransform(this);
	mesh = new ComponentMesh(this);
	texture = new ComponentTexture(this);
	camera = nullptr;

	isVisibleInFrustum = true;
	culledLastFrame = false;
}

GameObject::~GameObject()
{
	for (int i = 0; i < components.size(); i++)
	{
		delete components[i];
	}
	components.clear();

	if (parent != nullptr) {

		for (int i = 0; i < parent->children.size(); i++)
		{
			if (parent->children[i] == this) {
				parent->children.erase(parent->children.begin() + i);
			}
			break;
		}
	}
	parent = nullptr;

	while (!children.empty())
	{
		delete children[0];
	}
	children.clear();
}

Component* GameObject::AddComponent(Component* component)
{
	components.push_back(component);

	if (component->type == ComponentType::CAMERA)
	{
		camera = static_cast<ComponentCamera*>(component);
	}
	return component;
}

Component* GameObject::GetComponent(ComponentType type)
{
	for (auto it = components.begin(); it != components.end(); ++it) {
		if ((*it)->type == type) {
			return (*it);
		}
	}

	return nullptr;
}
