#include "Texture.h"
#include "ConsoleWindow.h"
#include <iostream>

Texture::Texture() {
    static bool initialized = false;
    if (!initialized) {
        ConsoleWindow::AddLog("Initializing DevIL", LogType::INFO);
        ilInit();
        iluInit();
        ilEnable(IL_ORIGIN_SET);
        initialized = true;
        ConsoleWindow::AddLog("DevIL initialized successfully", LogType::INFO);
    }
}

Texture::~Texture() {
    Unload();
}

bool Texture::LoadFromFile(const std::string& path, bool flipY) {
    ConsoleWindow::AddLog("Loading texture: " + path, LogType::INFO);

    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage(path.c_str())) {
        ConsoleWindow::AddLog("Error loading image with DevIL: " + path, LogType::ERROR_LOG);
        ilDeleteImages(1, &imageID);
        return false;
    }

    if (flipY)
        iluFlipImage();

    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    unsigned char* data = ilGetData();

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    ilDeleteImages(1, &imageID);

    filePath = path;

    char msg[256];
    snprintf(msg, sizeof(msg), "Texture loaded: %s (%dx%d, ID: %u)",
        path.c_str(), width, height, textureID);
    ConsoleWindow::AddLog(msg, LogType::INFO);

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