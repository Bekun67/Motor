#include "ComponentTexture.h"
#include <iostream>
#include <IL/il.h>
#include <IL/ilu.h>
#include <glad/glad.h>
#include "TextureImporter.h"

ComponentTexture::ComponentTexture(GameObject* gameObject)
    : Component(gameObject, ComponentType::TEXTURE),
    texturedata(nullptr),
    hasTexture(false)
{
}

ComponentTexture::~ComponentTexture()
{
    //clean texture
    if (texturedata != nullptr)
    {
        if (texturedata->id != 0)
        {
            glDeleteTextures(1, &texturedata->id);
        }
        delete texturedata;
        texturedata = nullptr;
    }
}

void ComponentTexture::Update()
{
}

bool ComponentTexture::LoadTexture(const std::string& path)
{
    //init devil in case not loaded
    static bool devilInitialized = false;
    if (!devilInitialized) {
        ilInit();
        iluInit();
        ilEnable(IL_ORIGIN_SET);
        devilInitialized = true;
    }

    //if there is a previous texture delete it
    if (texturedata != nullptr) {
        if (texturedata->id != 0) {
            glDeleteTextures(1, &texturedata->id);
        }
        delete texturedata;
        texturedata = nullptr;
    }

    //load image
    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage((const ILstring)path.c_str())) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        ilDeleteImages(1, &imageID);
        return false;
    }

    iluFlipImage();
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    unsigned char* data = ilGetData();

    //load texture
    GLuint newTextureID;
    glGenTextures(1, &newTextureID);
    glBindTexture(GL_TEXTURE_2D, newTextureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    ilDeleteImages(1, &imageID);

    //create texture data for this obj
    texturedata = new TextureData();
    texturedata->id = newTextureID;
    texturedata->type = "diffuse";
    texturedata->path = path;

    texturePath = path;
    hasTexture = true;

    std::cout << "Texture loaded for GameObject: " << path << " (ID: " << newTextureID << ")" << std::endl;
    return true;
}

unsigned int ComponentTexture::GetTextureID() const
{
    if (texturedata != nullptr)
    {
        return texturedata->id;
    }
    return 0;
}

bool ComponentTexture::LoadTextureFromCustomFormat(const std::string& path)
{
    CustomTexture customTex;
    if (!TextureImporter::LoadTexture(customTex, path)) {
        std::cerr << "Failed to load custom texture: " << path << std::endl;
        return false;
    }

    // Delete previous texture if exists
    if (texturedata != nullptr) {
        if (texturedata->id != 0) {
            glDeleteTextures(1, &texturedata->id);
        }
        delete texturedata;
        texturedata = nullptr;
    }

    // Create OpenGL texture
    GLuint newTextureID;
    glGenTextures(1, &newTextureID);
    glBindTexture(GL_TEXTURE_2D, newTextureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        customTex.width, customTex.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, customTex.data.data());

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Create texture data
    texturedata = new TextureData();
    texturedata->id = newTextureID;
    texturedata->type = "diffuse";
    texturedata->path = path;

    texturePath = path;
    hasTexture = true;

    std::cout << "Custom texture loaded: " << path << " (ID: " << newTextureID << ")" << std::endl;
    return true;
}