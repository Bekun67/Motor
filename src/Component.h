#pragma once
#include <map>
#include <string>
#include <variant>

class GameObject;

enum class ComponentType
{
    TRANSFORM,
    MESH,
    TEXTURE,
    CAMERA,
    UNKNOWN
};

using PropertyMap = std::map<std::string, std::variant<bool, int, float, double, std::string>>;

class Component
{
public:
    Component(GameObject* gameObject, ComponentType type);
    virtual ~Component();

    virtual void Update() = 0;
    virtual void Enable() { active = true; }
    virtual void Disable() { active = false; }

    ComponentType GetType() const { return type; }
    GameObject* GetGameObject() const { return gameObject; }
    bool IsActive() const { return active; }

    // Serialization methods
    virtual PropertyMap Serialize() const = 0;
    virtual void Deserialize(const PropertyMap& props) = 0;

public:
    ComponentType type;
    bool active;

protected:
    GameObject* gameObject;
};