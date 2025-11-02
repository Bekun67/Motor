#include "Component.h"
#include "GameObject.h"

Component::Component(GameObject* gameObject, ComponentType type)
    : gameObject(gameObject), type(type), active(true)
{
}

Component::~Component()
{
    gameObject = nullptr;
}