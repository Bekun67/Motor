#include "ComponentTexture.h"
#include "Application.h"
#include <iostream>

ComponentTexture::ComponentTexture(GameObject* gameObject)
    : Component(gameObject, ComponentType::TEXTURE),
    texturedata(nullptr),
    hasTexture(false)
{
}

ComponentTexture::~ComponentTexture()
{
    texturedata = nullptr;
}

void ComponentTexture::Update()
{
    // Update logic if needed
}

bool ComponentTexture::LoadTexture(const std::string& path)
{
    Texture* tex = Application::GetInstance().texture.get();

    if (tex->LoadFromFile(path))
    {
        texturePath = path;
        hasTexture = true;

        // Create TextureData if needed
        if (texturedata == nullptr)
        {
            texturedata = new TextureData();
        }

        texturedata->id = tex->GetID();
        texturedata->type = "diffuse";
        texturedata->path = path;

        std::cout << "Texture loaded for GameObject: " << path << std::endl;
        return true;
    }

    std::cerr << "Failed to load texture: " << path << std::endl;
    return false;
}

unsigned int ComponentTexture::GetTextureID() const
{
    if (texturedata != nullptr)
    {
        return texturedata->id;
    }
    return 0;
}