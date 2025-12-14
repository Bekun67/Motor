#pragma once
#include "Component.h"
#include "GameObject.h"
#include <string>

class ComponentTexture : public Component
{
public:
    ComponentTexture(GameObject* gameObject);
    virtual ~ComponentTexture();

    void Update() override;

    // Load texture from file
    bool LoadTexture(const std::string& path);

    // Get texture ID
    unsigned int GetTextureID() const;

    bool LoadTextureFromCustomFormat(const std::string& path);

    // Serialization
    PropertyMap Serialize() const override;
    void Deserialize(const PropertyMap& props) override;

public:
    TextureData* texturedata;
    std::string texturePath;
    bool hasTexture;
    bool hasTransparency = false;

	void CreateCheckerboardTexture();
};