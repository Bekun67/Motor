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

	isVisibleInFrustum = true;
	culledLastFrame = false;
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

	isVisibleInFrustum = true;
	culledLastFrame = false;
}

GameObject::~GameObject()
{
	// CRITICAL: Delete children FIRST before touching anything else
	// This prevents children from trying to access their parent during deletion
	if (!children.empty())
	{
		std::vector<GameObject*> childrenCopy = children;
		children.clear();

		for (GameObject* child : childrenCopy)
		{
			if (child != nullptr)
			{
				child->parent = nullptr; // Prevent child from modifying parent's list
				delete child; // This will recursively delete grandchildren
			}
		}
	}

	// Remove from parent's children list AFTER children are deleted
	if (parent != nullptr)
	{
		for (int i = 0; i < parent->children.size(); i++)
		{
			if (parent->children[i] == this)
			{
				parent->children.erase(parent->children.begin() + i);
				break;
			}
		}
		parent = nullptr;
	}

	// Delete the three main components (they are NOT in the components vector)
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

	// Delete any additional components in the components vector
	for (int i = 0; i < components.size(); i++)
	{
		if (components[i] != nullptr)
		{
			delete components[i];
			components[i] = nullptr;
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