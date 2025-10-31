#include "ComponentMesh.h"
#include "Application.h"

ComponentMesh::ComponentMesh(GameObject* gameObject) : Component(gameObject, ComponentType::MESH), meshdata(nullptr)
{
}

ComponentMesh::~ComponentMesh()
{ 
    meshdata = nullptr;
}

void ComponentMesh::Update()
{

}

void ComponentMesh::Draw(Camera* camera)
{
    
}

