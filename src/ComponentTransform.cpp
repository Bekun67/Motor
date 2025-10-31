#include "ComponentTransform.h"
#include "GameObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

ComponentTransform::ComponentTransform(GameObject* gameObject) : Component(gameObject, ComponentType::TRANSFORM)
{
}

ComponentTransform::~ComponentTransform()
{

}

void ComponentTransform::Update()
{
}

void ComponentTransform::SetFromNode(const aiNode* node)
{
   node->mTransformation.Decompose(scaling, rotation, translation);

    //float3 pos(translation.x, translation.y, translation.z);
    //float3 scale(scaling.x, scaling.y, scaling.z);
    //Quat rot(rotation.x, rotation.y, rotation.z, rotation.w);

}