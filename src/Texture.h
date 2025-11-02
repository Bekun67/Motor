#pragma once
#include "Module.h"
#include <string>
#include <glad/glad.h>
#include <IL/il.h>
#include <IL/ilu.h>

class Texture : public Module
{
public:
    Texture();
    ~Texture();

    // Load texture with DevIL
    bool LoadFromFile(const std::string& path, bool flipY = true);

    // Activate texture with a given slot 
    void Bind(GLenum textureUnit = GL_TEXTURE0) const;

	// Unload texture from GPU
    void Unload();

    GLuint GetID() const { return textureID; }

private:
    GLuint textureID = 0;
    std::string filePath;
    void CreateCheckerboard();
};