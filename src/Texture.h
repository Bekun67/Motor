//#pragma once
//
//#include <glad/glad.h>
//#include <string>
//
//class Texture
//{
//public:
//    Texture();
//    ~Texture();
//
//    // Carga una textura checkerboard procedural
//    bool LoadCheckerboard(int width = 64, int height = 64);
//
//    // Carga una textura desde un archivo usando DevIL
//    bool Load(const char* filePath);
//
//    // Enlaza la textura para usarla en el render
//    void Bind(unsigned int textureUnit = 0) const;
//
//    // Desenlaza la textura
//    void Unbind() const;
//
//    // Limpia los recursos
//    void CleanUp();
//
//    // Getters
//    GLuint GetTextureID() const { return textureID; }
//    int GetWidth() const { return width; }
//    int GetHeight() const { return height; }
//    const char* GetPath() const { return path.c_str(); }
//
//private:
//    GLuint textureID;
//    int width;
//    int height;
//    std::string path;
//
//    // Configura los parámetros de la textura
//    void SetTextureParameters();
//};