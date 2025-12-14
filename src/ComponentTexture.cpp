#include "ComponentTexture.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include <glad/glad.h>
#include "TextureImporter.h"
#include "fileSystemManager.h"
#include "Structures.h"
#include "LoadFBX.h"
#include <iostream>

ComponentTexture::ComponentTexture(GameObject* gameObject)
    : Component(gameObject, ComponentType::TEXTURE),
    texturedata(nullptr),
    hasTexture(false),
    hasTransparency(false)
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

    //check for transparency
    hasTransparency = false;
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; i++) {
        unsigned char alpha = data[i * 4 + 3];
        if (alpha < 255) {
            hasTransparency = true;
            break;
        }
    }

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

    if (hasTransparency) {
        std::cout << "Texture loaded with transparency: " << path << " (ID: " << newTextureID << ")" << std::endl;
    }
    else {
        std::cout << "Texture loaded: " << path << " (ID: " << newTextureID << ")" << std::endl;
    }    return true;
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

    hasTransparency = false;
    int totalPixels = customTex.width * customTex.height;
    for (int i = 0; i < totalPixels; i++) 
    {
        unsigned char alpha = customTex.data[i * 4 + 3];
        if (alpha < 255) 
        {
            hasTransparency = true;
            break;
        }
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

    if (hasTransparency) 
    {
        std::cout << "Custom texture loaded WITH transparency: " << path
            << " (ID: " << newTextureID << ")" << std::endl;
    }
    else 
    {
        std::cout << "Custom texture loaded WITHOUT transparency: " << path
            << " (ID: " << newTextureID << ")" << std::endl;
    }

    return true;
}

PropertyMap ComponentTexture::Serialize() const
{
    PropertyMap props;
    props["texturePath"] = texturePath;
    props["hasTexture"] = hasTexture;
    props["hasTransparency"] = hasTransparency;
    return props;
}

void ComponentTexture::Deserialize(const PropertyMap& props)
{
    if (props.count("hasTexture")) hasTexture = std::get<bool>(props.at("hasTexture"));
    if (props.count("hasTransparency")) hasTransparency = std::get<bool>(props.at("hasTransparency"));

    if (props.count("texturePath")) {
        texturePath = std::get<std::string>(props.at("texturePath"));

        if (!texturePath.empty())
        {
            // Check if it's a checkerboard texture
            if (texturePath == "checkerboard")
            {
                CreateCheckerboardTexture();
            }
            else
            {
                // Try to load custom texture format first
                std::string customTexturePath = TextureImporter::GetCustomTexturePath(texturePath);

                bool loaded = false;

                // First try to load from custom format
                if (FileSystemManager::FileExists(customTexturePath))
                {
                    loaded = LoadTextureFromCustomFormat(customTexturePath);
                    if (loaded)
                    {
                        std::cout << "Loaded texture from custom format: " << customTexturePath << std::endl;
                    }
                }

                // If custom format doesn't exist or failed, try original format
                if (!loaded && FileSystemManager::FileExists(texturePath))
                {
                    loaded = LoadTexture(texturePath);
                    if (loaded)
                    {
                        std::cout << "Loaded texture from original format: " << texturePath << std::endl;
                    }
                }

                // If both failed, use checkerboard as fallback
                if (!loaded)
                {
                    std::cout << "Failed to load texture: " << texturePath << ", using checkerboard fallback" << std::endl;
                    CreateCheckerboardTexture();
                }
            }
        }
        else
        {
            // Empty texture path, create checkerboard
            CreateCheckerboardTexture();
        }
    }
    else
    {
        // No texture path property, create checkerboard
        CreateCheckerboardTexture();
    }
}

void ComponentTexture::CreateCheckerboardTexture()
{
	// Delete previous texture if exists
	if (texturedata != nullptr && texturedata->id != 0) {
		glDeleteTextures(1, &texturedata->id);
	}

	const int size = 64;
	GLubyte checkerImage[64][64][4];
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			int c = ((((i & 0x8) == 0) ^ ((j & 0x8) == 0))) * 255;
			checkerImage[i][j][0] = (GLubyte)c;
			checkerImage[i][j][1] = (GLubyte)c;
			checkerImage[i][j][2] = (GLubyte)c;
			checkerImage[i][j][3] = (GLubyte)255;
		}
	}

	GLuint checkID;
	glGenTextures(1, &checkID);
	glBindTexture(GL_TEXTURE_2D, checkID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	hasTexture = true;
	if (texturedata == nullptr) {
		texturedata = new TextureData();
	}
	texturedata->id = checkID;
	texturedata->type = "checkerboard";
	texturedata->path = "checkerboard";
	texturePath = "checkerboard";

	std::cout << "Created checkerboard texture (ID: " << checkID << ")" << std::endl;
}