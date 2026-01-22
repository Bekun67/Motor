#pragma once
#include <vector>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "ComponentCollider.h"
#include "ComponentHingeConstraint.h"
#include "ComponentSliderConstraint.h"
#include "ComponentDistanceConstraint.h"
#include "ComponentConeConstraint.h"
#include "ComponentFirstPersonController.h"

class Component;
enum class ComponentType;

class GameObject {
public:
    GameObject(const std::string& name = "GameObject");
    ~GameObject();

    Component* CreateComponent(ComponentType type);

    ComponentCollider* CreateCollider(ColliderType colliderType);

    Component* GetComponent(ComponentType type) const;

    std::vector<Component*> GetComponentsOfType(ComponentType type) const;

    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    void SetParent(GameObject* newParent);
    void InsertChildAt(GameObject* child, int index);
    int GetChildIndex(GameObject* child) const;

    void Update();

    const std::string& GetName() const { return name; }
    void SetName(const std::string& newName) { name = newName; }
    bool IsActive() const { return active; }
    void SetActive(bool state) { active = state; }
    GameObject* GetParent() const { return parent; }
    const std::vector<GameObject*>& GetChildren() const { return children; }
    const std::vector<Component*>& GetComponents() const { return components; }

    void MarkForDeletion() { markedForDeletion = true; }
    bool IsMarkedForDeletion() const { return markedForDeletion; }

    bool IsBeingDestroyed() const { return isBeingDestroyed; }

    // Serialization
    void Serialize(nlohmann::json& gameObjectArray) const;
    static GameObject* Deserialize(const nlohmann::json& gameObjectObj, GameObject* parent = nullptr);

    static void AssignSerializationIndices(GameObject* root, int& currentIndex);
    static void CollectAllGameObjects(GameObject* root, std::vector<GameObject*>& outList);
    static void ResolveConstraintReferences(const std::vector<GameObject*>& allGameObjects);

    int GetSerializationIndex() const { return serializationIndex; }
    void SetSerializationIndex(int index) { serializationIndex = index; }

    void RemoveComponent(Component* component);

    ComponentHingeConstraint* CreateHingeConstraint();
    ComponentSliderConstraint* CreateSliderConstraint();
    ComponentDistanceConstraint* CreateDistanceConstraint();
    ComponentConeConstraint* CreateConeConstraint();
	ComponentFirstPersonController* CreateFirstPersonController();

    void CreatePendingConstraints();

public:
    std::string name;
    bool active = true;
    bool isPrimitive = false;

    void NotifyConstraintsBeforeDestruction();

    std::vector<nlohmann::json> pendingConstraints;

private:
    GameObject* parent = nullptr;
    std::vector<GameObject*> children;

    std::vector<Component*> components;
    std::vector<std::unique_ptr<Component>> componentOwners;

    bool markedForDeletion = false;
    bool isBeingDestroyed = false;
    int serializationIndex = -1;

    static void NotifyConstraintsRecursive(GameObject* current, GameObject* deletedObject);
};