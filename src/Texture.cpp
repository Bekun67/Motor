#include "Texture.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>
#include <iostream>

Texture::Texture() : textureID(0), width(0), height(0)
{
}

Texture::~Texture()
{
    CleanUp();
}

bool Texture::LoadCheckerboard(int w, int h)
{
    width = w;
    height = h;
    path = "checkerboard";

    // Crear el patrón de checkerboard proceduralmente (formato del PDF)
    GLubyte(*checkerImage)[64][4] = new GLubyte[h][64][4];

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int c = ((((i & 0x8) == 0) ^ (((j & 0x8)) == 0))) * 255;
            checkerImage[i][j][0] = (GLubyte)c;
            checkerImage[i][j][1] = (GLubyte)c;
            checkerImage[i][j][2] = (GLubyte)c;
            checkerImage[i][j][3] = (GLubyte)255;
        }
    }

    // Generar la textura en OpenGL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configurar parámetros de textura
    SetTextureParameters();

    // Cargar los datos de la textura
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);

    // Generar mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] checkerImage;

    std::cout << "Checkerboard texture created: " << width << "x" << height << std::endl;
    return true;
}

bool Texture::Load(const char* filePath)
{
    // Inicializar DevIL si no está inicializado
    static bool devilInitialized = false;
    if (!devilInitialized) {
        ilInit();
        iluInit();
        ilutInit();
        ilutRenderer(ILUT_OPENGL);
        devilInitialized = true;
    }

    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    // Cargar la imagen
    if (!ilLoadImage(filePath)) {
        ILenum error = ilGetError();
        std::cerr << "Failed to load texture: " << filePath << " - Error: " << error << std::endl;
        ilDeleteImages(1, &imageID);
        return false;
    }

    // Convertir la imagen a un formato que OpenGL entienda
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
        std::cerr << "Failed to convert image: " << filePath << std::endl;
        ilDeleteImages(1, &imageID);
        return false;
    }

    // Obtener información de la imagen
    width = ilGetInteger(IL_IMAGE_WIDTH);
    height = ilGetInteger(IL_IMAGE_HEIGHT);
    path = filePath;

    // Generar textura en OpenGL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configurar parámetros
    SetTextureParameters();

    // Cargar los datos de la imagen
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());

    // Generar mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Limpiar recursos de DevIL
    ilDeleteImages(1, &imageID);

    std::cout << "Texture loaded: " << filePath << " (" << width << "x" << height << ")" << std::endl;
    return true;
}

void Texture::SetTextureParameters()
{
    // Configurar wrapping (repetición)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Configurar filtrado
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
}

void Texture::Bind(unsigned int textureUnit) const
{
    if (textureID != 0) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
}

void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::CleanUp()
{
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}