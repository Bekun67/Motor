#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentCamera.h"
#include "ComponentRotate.h"
#include "ComponentRigidBody.h"
#include "ComponentCollider.h"
#include "ComponentHingeConstraint.h"
#include <nlohmann/json.hpp>

GameObject::GameObject(const std::string& name) : name(name), active(true), parent(nullptr) {
    CreateComponent(ComponentType::TRANSFORM);
}

GameObject::~GameObject() {

    isBeingDestroyed = true;

    NotifyConstraintsBeforeDestruction();

    for (auto* comp : components) {
        if (comp->GetType() == ComponentType::CONSTRAINT) {
            comp->Disable();
        }
    }

    for (auto* comp : components) {
        if (comp->GetType() == ComponentType::RIGIDBODY) {
            comp->Disable();
        }
    }
}

Component* GameObject::CreateComponent(ComponentType type) {
    Component* newComponent = nullptr;

    switch (type) {
    case ComponentType::TRANSFORM:
        if (GetComponent(ComponentType::TRANSFORM) != nullptr) {
            return GetComponent(ComponentType::TRANSFORM);
        }
        newComponent = new Transform(this);
        break;

    case ComponentType::MESH:
        newComponent = new ComponentMesh(this);
        break;

    case ComponentType::MATERIAL:
        if (GetComponent(ComponentType::MATERIAL) != nullptr) {
            return GetComponent(ComponentType::MATERIAL);
        }
        newComponent = new ComponentMaterial(this);
        break;

    case ComponentType::CAMERA:
        if (GetComponent(ComponentType::CAMERA) != nullptr) {
            return GetComponent(ComponentType::CAMERA);
        }
        newComponent = new ComponentCamera(this);
        break;

    case ComponentType::ROTATE:
        newComponent = new ComponentRotate(this);
        break;

    case ComponentType::RIGIDBODY:
        if (GetComponent(ComponentType::RIGIDBODY) != nullptr) {
            LOG_CONSOLE("GameObject '%s' already has a RigidBody component!", name.c_str());
            return GetComponent(ComponentType::RIGIDBODY);
        }
        newComponent = new ComponentRigidBody(this);
        break;

    case ComponentType::COLLIDER:
        newComponent = new ComponentCollider(this, ColliderType::BOX);
        break;
    case ComponentType::CONSTRAINT:
        newComponent = new ComponentHingeConstraint(this);
        break;
    case ComponentType::FIRSTPERSON:
        if (GetComponent(ComponentType::FIRSTPERSON) != nullptr) {
            LOG_CONSOLE("GameObject '%s' already has a FirstPersonController component!", name.c_str());
            return GetComponent(ComponentType::FIRSTPERSON);
        }
        newComponent = new ComponentFirstPersonController(this);
        break;

    default:
        LOG_DEBUG("ERROR: Unknown component type requested for GameObject '%s'", name.c_str());
        LOG_CONSOLE("Failed to create component");
        return nullptr;
    }

    if (newComponent) {
        componentOwners.push_back(std::unique_ptr<Component>(newComponent));
        components.push_back(newComponent);
    }

    return newComponent;
}

ComponentHingeConstraint* GameObject::CreateHingeConstraint()
{
    ComponentHingeConstraint* constraint = new ComponentHingeConstraint(this);

    if (constraint) {
        componentOwners.push_back(std::unique_ptr<Component>(constraint));
        components.push_back(constraint);
    }

    return constraint;
}

ComponentSliderConstraint* GameObject::CreateSliderConstraint()
{
    ComponentSliderConstraint* constraint = new ComponentSliderConstraint(this);

    if (constraint) {
        componentOwners.push_back(std::unique_ptr<Component>(constraint));
        components.push_back(constraint);
    }

    return constraint;
}

ComponentDistanceConstraint* GameObject::CreateDistanceConstraint()
{
    ComponentDistanceConstraint* constraint = new ComponentDistanceConstraint(this);

    if (constraint) {
        componentOwners.push_back(std::unique_ptr<Component>(constraint));
        components.push_back(constraint);
    }

    return constraint;
}

ComponentConeConstraint* GameObject::CreateConeConstraint()
{
    ComponentConeConstraint* constraint = new ComponentConeConstraint(this);

    if (constraint) {
        componentOwners.push_back(std::unique_ptr<Component>(constraint));
        components.push_back(constraint);
    }

    return constraint;
}

ComponentCollider* GameObject::CreateCollider(ColliderType colliderType)
{
    ComponentCollider* collider = new ComponentCollider(this, colliderType);

    if (collider) {
        componentOwners.push_back(std::unique_ptr<Component>(collider));
        components.push_back(collider);
    }

    return collider;
}

ComponentFirstPersonController* GameObject::CreateFirstPersonController()
{
    ComponentFirstPersonController* controller = new ComponentFirstPersonController(this);

    if (controller) {
        componentOwners.push_back(std::unique_ptr<Component>(controller));
        components.push_back(controller);
    }

    return controller;
}

Component* GameObject::GetComponent(ComponentType type) const {
    for (auto* comp : components) {
        if (comp->GetType() == type) {
            return comp;
        }
    }
    return nullptr;
}

std::vector<Component*> GameObject::GetComponentsOfType(ComponentType type) const {
    std::vector<Component*> result;
    for (auto* comp : components) {
        if (comp->GetType() == type) {
            result.push_back(comp);
        }
    }
    return result;
}

void GameObject::AddChild(GameObject* child) {
    if (child && child != this) {
        if (child->parent) {
            child->parent->RemoveChild(child);
        }

        child->parent = this;
        children.push_back(child);
    }
}

void GameObject::RemoveChild(GameObject* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        (*it)->parent = nullptr;
        children.erase(it);
    }
}

void GameObject::SetParent(GameObject* newParent) {
    if (newParent) {
        newParent->AddChild(this);
    }
    else if (parent) {
        parent->RemoveChild(this);
    }
}

void GameObject::InsertChildAt(GameObject* child, int index) {
    if (child && child != this) {
        if (child->parent) {
            child->parent->RemoveChild(child);
        }

        child->parent = this;

        if (index < 0) index = 0;
        if (index > static_cast<int>(children.size())) index = static_cast<int>(children.size());

        // Insert child
        children.insert(children.begin() + index, child);
    }
}

int GameObject::GetChildIndex(GameObject* child) const {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        return static_cast<int>(std::distance(children.begin(), it));
    }
    return -1;
}

void GameObject::Update()
{
    if (!active || isBeingDestroyed || markedForDeletion) return;

    for (auto* component : components)
    {
        if (component && component->IsActive())
        {
            component->Update();
        }
    }

    // Create a copy to avoid iterator invalidation
    std::vector<GameObject*> childrenCopy = children;

    for (auto* child : childrenCopy)
    {
        if (child && !child->IsMarkedForDeletion())
        {
            child->Update();
        }
    }
}

void GameObject::Serialize(nlohmann::json& gameObjectArray) const {
    // Create a JSON object
    nlohmann::json gameObjectObj;

    // Set the name and active state
    gameObjectObj["name"] = name;
    gameObjectObj["active"] = active;
    gameObjectObj["isprimitive"] = isPrimitive;

    // Components
    nlohmann::json componentsArray = nlohmann::json::array();

    for (const auto* component : components) {
        nlohmann::json componentObj;
        componentObj["type"] = static_cast<int>(component->GetType());
        componentObj["active"] = component->IsActive();
        component->Serialize(componentObj);
        componentsArray.push_back(componentObj);
    }

    gameObjectObj["components"] = componentsArray;

    // Children
    nlohmann::json childrenArray = nlohmann::json::array();
    for (const auto* child : children) {
        child->Serialize(childrenArray);
    }
    gameObjectObj["children"] = childrenArray;

    gameObjectArray.push_back(gameObjectObj);
}

GameObject* GameObject::Deserialize(const nlohmann::json& gameObjectObj, GameObject* parent) {
    if (!gameObjectObj.is_object()) {
        return nullptr;
    }

    // Create gameobject
    std::string objName = gameObjectObj.contains("name") ? gameObjectObj["name"].get<std::string>() : "GameObject";
    GameObject* newObject = new GameObject(objName);

    if (gameObjectObj.contains("active")) {
        newObject->SetActive(gameObjectObj["active"].get<bool>());
    }

    if (gameObjectObj.contains("isprimitive")) {
        newObject->isPrimitive = gameObjectObj["isprimitive"].get<bool>();
    }

    if (parent) {
        parent->AddChild(newObject);
    }

    // Components
    if (gameObjectObj.contains("components") && gameObjectObj["components"].is_array()) {
        const nlohmann::json& componentsArray = gameObjectObj["components"];

        for (const auto& componentObj : componentsArray) {
            if (!componentObj.contains("type")) continue;

            ComponentType type = static_cast<ComponentType>(componentObj["type"].get<int>());

            if (type == ComponentType::CONSTRAINT) {
                continue;
            }

            Component* component = nullptr;
            if (type == ComponentType::TRANSFORM) {
                component = newObject->GetComponent(ComponentType::TRANSFORM);
            }
            else if (type == ComponentType::FIRSTPERSON) {
                component = newObject->CreateFirstPersonController();
            }
            else if (type == ComponentType::COLLIDER) {
                if (componentObj.contains("colliderType")) {
                    int colliderTypeInt = componentObj["colliderType"].get<int>();
                    ColliderType colliderType = static_cast<ColliderType>(colliderTypeInt);
                    component = newObject->CreateCollider(colliderType);
                }
                else {
                    component = newObject->CreateComponent(type);
                }
            }
            else {
                component = newObject->CreateComponent(type);
            }

            if (component) {
                if (componentObj.contains("active")) {
                    component->SetActive(componentObj["active"].get<bool>());
                }
                component->Deserialize(componentObj);
            }
        }


        for (const auto& componentObj : componentsArray) {
            if (!componentObj.contains("type")) continue;

            ComponentType type = static_cast<ComponentType>(componentObj["type"].get<int>());

            if (type == ComponentType::CONSTRAINT) {

                newObject->pendingConstraints.push_back(componentObj);
            }
        }
    }

    // Children
    if (gameObjectObj.contains("children") && gameObjectObj["children"].is_array()) {
        const nlohmann::json& childrenArray = gameObjectObj["children"];
        for (const auto& childObj : childrenArray) {
            Deserialize(childObj, newObject);
        }
    }

    return newObject;
}

void GameObject::AssignSerializationIndices(GameObject* root, int& currentIndex)
{
    if (!root) return;

    root->serializationIndex = currentIndex++;

    for (GameObject* child : root->GetChildren())
    {
        AssignSerializationIndices(child, currentIndex);
    }
}

void GameObject::CollectAllGameObjects(GameObject* root, std::vector<GameObject*>& outList)
{
    if (!root) return;

    outList.push_back(root);

    for (GameObject* child : root->GetChildren())
    {
        CollectAllGameObjects(child, outList);
    }
}

void GameObject::ResolveConstraintReferences(const std::vector<GameObject*>& allGameObjects)
{
    for (GameObject* obj : allGameObjects)
    {
        if (!obj) continue;

        std::vector<Component*> constraints = obj->GetComponentsOfType(ComponentType::CONSTRAINT);
        for (Component* comp : constraints)
        {
            ComponentConstraint* constraint = static_cast<ComponentConstraint*>(comp);
            if (constraint)
            {
                constraint->ResolveConnectedBodyReference(allGameObjects);
            }
        }
    }
}

void GameObject::RemoveComponent(Component* component)
{
    if (!component) return;

	// If it is a RigidBody, notify all attached colliders
    if (component->GetType() == ComponentType::RIGIDBODY)
    {
        std::vector<Component*> colliders = GetComponentsOfType(ComponentType::COLLIDER);
        for (Component* comp : colliders)
        {
            ComponentCollider* collider = static_cast<ComponentCollider*>(comp);
            if (collider && collider->IsActive())
            {
                collider->ForceStandaloneMode();
            }
        }
    }

	// Deactivate before removal
    component->Disable();

    auto it = std::find(components.begin(), components.end(), component);
    if (it != components.end())
    {
        components.erase(it);
    }

    auto ownerIt = std::find_if(componentOwners.begin(), componentOwners.end(),
        [component](const std::unique_ptr<Component>& ptr) {
            return ptr.get() == component;
        });

    if (ownerIt != componentOwners.end())
    {
        componentOwners.erase(ownerIt);
    }
}

void GameObject::NotifyConstraintsBeforeDestruction()
{
    //get root to search all go
    GameObject* root = this;
    while (root->parent != nullptr)
    {
        root = root->parent;
    }

    //notify all constraints
    NotifyConstraintsRecursive(root, this);
}

void GameObject::NotifyConstraintsRecursive(GameObject* current, GameObject* deletedObject)
{
    if (!current) return;

    //check all go constraints
    std::vector<Component*> constraints = current->GetComponentsOfType(ComponentType::CONSTRAINT);
    for (Component* comp : constraints)
    {
        ComponentConstraint* constraint = static_cast<ComponentConstraint*>(comp);
        if (constraint && constraint->GetConnectedBody() == deletedObject)
        {
            constraint->OnConnectedBodyInvalidated();
            LOG_DEBUG("[GameObject] Notified constraint on '%s' that connected body '%s' is being deleted",
                current->GetName().c_str(), deletedObject->GetName().c_str());
        }
    }

    for (GameObject* child : current->GetChildren())
    {
        NotifyConstraintsRecursive(child, deletedObject);
    }
}

void GameObject::CreatePendingConstraints() {

    for (const auto& componentObj : pendingConstraints) {
        if (!componentObj.contains("constraintType")) continue;

        int constraintTypeInt = componentObj["constraintType"].get<int>();
        ConstraintType constraintType = static_cast<ConstraintType>(constraintTypeInt);

        Component* component = nullptr;
        switch (constraintType) {
        case ConstraintType::HINGE:
            component = CreateHingeConstraint();
            break;
        case ConstraintType::SLIDER:
            component = CreateSliderConstraint();
            break;
        case ConstraintType::DISTANCE:
            component = CreateDistanceConstraint();
            break;
        case ConstraintType::CONE:
            component = CreateConeConstraint();
            break;
        default:
            component = CreateHingeConstraint();
            break;
        }

        if (component) {
            if (componentObj.contains("active")) {
                component->SetActive(componentObj["active"].get<bool>());
            }
            component->Deserialize(componentObj);
        }
    }

    pendingConstraints.clear();

    for (GameObject* child : children) {
        child->CreatePendingConstraints();
    }
}