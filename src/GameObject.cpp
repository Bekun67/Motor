#include "GameObject.h"
#include "Component.h"
#include <algorithm>

GameObject::GameObject()
{
	m_UUID = UUID();
	m_ParentUUID = UUID(0);
	name = "gameObject";
	parent = nullptr;

	transform = new ComponentTransform(this);
	mesh = new ComponentMesh(this);
	texture = new ComponentTexture(this);
	camera = nullptr;
}

GameObject::GameObject(GameObject* parent)
{
	m_UUID = UUID();
	name = "gameObject";
	this->parent = parent;

	if (parent != nullptr) {
		m_ParentUUID = parent->GetUUID();
		parent->children.push_back(this);
	}
	else {
		m_ParentUUID = UUID(0);
	}

	transform = new ComponentTransform(this);
	mesh = new ComponentMesh(this);
	texture = new ComponentTexture(this);
	camera = nullptr;
}

GameObject::~GameObject()
{
	// Prevent multiple deletes
	if (m_IsBeingDestroyed)
		return;

	m_IsBeingDestroyed = true;

	DestroyHierarchy();
}

void GameObject::DestroyHierarchy()
{
	if (m_IsBeingDestroyed && children.empty() && parent == nullptr)
	{
		if (transform != nullptr)
		{
			delete transform;
			transform = nullptr;
		}

		if (mesh != nullptr)
		{
			delete mesh;
			mesh = nullptr;
		}

		if (texture != nullptr)
		{
			delete texture;
			texture = nullptr;
		}

		if (camera != nullptr)
		{
			delete camera;
			camera = nullptr;
		}

		for (Component* component : components)
		{
			if (component != nullptr)
			{
				delete component;
			}
		}
		components.clear();
		return;
	}

	// Delete children first 
	if (!children.empty())
	{
		std::vector<GameObject*> childrenCopy = children;
		children.clear(); 

		for (GameObject* child : childrenCopy)
		{
			if (child != nullptr && !child->m_IsBeingDestroyed)
			{
				child->parent = nullptr; 
				delete child;
			}
		}
	}

	// Remove from parent's children list 
	if (parent != nullptr && !m_IsBeingDestroyed)
	{
		auto it = std::find(parent->children.begin(), parent->children.end(), this);
		if (it != parent->children.end())
		{
			parent->children.erase(it);
		}
		parent = nullptr;
	}

	// Clean components
	if (transform != nullptr)
	{
		delete transform;
		transform = nullptr;
	}

	if (mesh != nullptr)
	{
		delete mesh;
		mesh = nullptr;
	}

	if (texture != nullptr)
	{
		delete texture;
		texture = nullptr;
	}

	if (camera != nullptr)
	{
		delete camera;
		camera = nullptr;
	}

	// Clean extra components
	for (Component* component : components)
	{
		if (component != nullptr)
		{
			delete component;
		}
	}
	components.clear();
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

void GameObject::SetParent(GameObject* newParent)
{
	// Remove from old parent
	if (parent != nullptr)
	{
		parent->RemoveChild(this);
	}

	// Set new parent
	parent = newParent;

	if (newParent != nullptr)
	{
		m_ParentUUID = newParent->GetUUID();
		newParent->AddChild(this);
	}
	else
	{
		m_ParentUUID = UUID(0);
	}
}

void GameObject::RemoveChild(GameObject* child)
{
	auto it = std::find(children.begin(), children.end(), child);
	if (it != children.end())
	{
		children.erase(it);
	}
}

void GameObject::AddChild(GameObject* child)
{
	// Check if already a child
	auto it = std::find(children.begin(), children.end(), child);
	if (it == children.end())
	{
		children.push_back(child);
	}
}

int GameObject::GetChildIndex() const
{
	if (parent == nullptr)
		return -1;

	for (int i = 0; i < parent->children.size(); i++)
	{
		if (parent->children[i] == this)
			return i;
	}

	return -1;
}

void GameObject::MoveUp()
{
	if (parent == nullptr)
		return;

	int index = GetChildIndex();
	if (index > 0)
	{
		// Swap with previous sibling
		std::swap(parent->children[index], parent->children[index - 1]);
	}
}

void GameObject::MoveDown()
{
	if (parent == nullptr)
		return;

	int index = GetChildIndex();
	if (index >= 0 && index < parent->children.size() - 1)
	{
		// Swap with next sibling
		std::swap(parent->children[index], parent->children[index + 1]);
	}
}

void GameObject::GetAllDescendants(std::vector<GameObject*>& descendants)
{
	for (GameObject* child : children)
	{
		descendants.push_back(child);
		child->GetAllDescendants(descendants);
	}
}

bool GameObject::IsDescendantOf(GameObject* potentialAncestor) const
{
	if (parent == nullptr)
		return false;

	if (parent == potentialAncestor)
		return true;

	return parent->IsDescendantOf(potentialAncestor);
}