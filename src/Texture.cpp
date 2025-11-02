#include "Texture.h"
#include <iostream>

// Para el checkerboard
#define CHECKERS_HEIGHT 64
#define CHECKERS_WIDTH 64

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

void Texture::CreateCheckerboard() {
    GLubyte checkerImage[CHECKERS_HEIGHT][CHECKERS_WIDTH][4];

    // Generar patrón de checkerboard
    for (int i = 0; i < CHECKERS_HEIGHT; i++) {
        for (int j = 0; j < CHECKERS_WIDTH; j++) {
            int c = ((((i & 0x8) == 0) ^ ((j & 0x8) == 0))) * 255;
            checkerImage[i][j][0] = (GLubyte)c;
            checkerImage[i][j][1] = (GLubyte)c;
            checkerImage[i][j][2] = (GLubyte)c;
            checkerImage[i][j][3] = (GLubyte)255;
        }
    }

    // Crear textura de checkerboard
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHECKERS_WIDTH, CHECKERS_HEIGHT, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);

    glGenerateMipmap(GL_TEXTURE_2D);

    // Parametros de textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Textura checkerboard creada" << std::endl;
}

bool Texture::LoadFromFile(const std::string& path, bool flipY) {
    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage(path.c_str())) {
        std::cerr << "Error loading image with DevIL: " << path << std::endl;
        std::cerr << "Generating checkerboard..." << std::endl;
        ilDeleteImages(1, &imageID);

        // Chekerboard si no carga
        CreateCheckerboard();
        filePath = path + " (checkerboard fallback)";
        return true; 
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
    std::cout << "Texture loaded correctly: " << path << std::endl;
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