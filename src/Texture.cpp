#include "Texture.h"
#include <iostream>

Texture::Texture() {
	// Initialize DevIL only once
    static bool initialized = false;
    if (!initialized) {
        ilInit();
        iluInit();
        ilEnable(IL_ORIGIN_SET);
        initialized = true;
    }
}

Texture::~Texture() {
    Unload();
}

bool Texture::LoadFromFile(const std::string& path, bool flipY) {
    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage(path.c_str())) {
        std::cerr << "Error loading image with DevIL: " << path << std::endl;
        ilDeleteImages(1, &imageID);
        return false;
    }

    if (flipY)
        iluFlipImage();

    // Convert to RGBA format of 8 bits
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    unsigned char* data = ilGetData();

    // create texture with OpenGL
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

	// Default texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    ilDeleteImages(1, &imageID);

    filePath = path;
    std::cout << "Textura cargada correctamente: " << path << std::endl;
    return true;
}

void Texture::Bind(GLenum textureUnit) const {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::Unload() {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}
