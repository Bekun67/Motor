#include "ComponentMesh.h"
#include "Application.h"

ComponentMesh::ComponentMesh(GameObject* gameObject) : Component(gameObject, ComponentType::MESH), mesh(nullptr)
{
}

ComponentMesh::~ComponentMesh()
{ 
    mesh = nullptr;
}

void ComponentMesh::Update()
{

}

void ComponentMesh::Draw(Camera* camera)
{
    
}

