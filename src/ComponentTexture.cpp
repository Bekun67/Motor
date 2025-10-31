#include "ComponentTexture.h"
#include "GameObject.h"

ComponentTexture::ComponentTexture(GameObject* gameObject) : Component(gameObject, ComponentType::TEXTURE)
{
}

ComponentTexture::~ComponentTexture()
{

}

void ComponentTexture::Update()
{
}

void ComponentTexture::LoadTexture(const aiScene* scene, const aiNode* node, unsigned int i)
{
    const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    unsigned int numTextures = material->GetTextureCount(aiTextureType_DIFFUSE);

    aiString path;
    if (numTextures > 0)
    {
        material->GetTexture(aiTextureType_DIFFUSE, 0, &path);
     
    }
}