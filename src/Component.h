#pragma once

class GameObject;

enum class ComponentType
{
    TRANSFORM,
    MESH,
    TEXTURE,
    CAMERA,
    UNKNOWN
};

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

public:
    ComponentType type;
    bool active;

protected:
    GameObject* gameObject;
};