#pragma once
#include <glad/glad.h>
#include <vector>
#include <string>

struct TextureData {
    GLuint id = 0;
    std::string type;
    std::string path;
};

struct MeshData {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei numIndices = 0;
    std::vector<TextureData> textures;
};

extern std::vector<MeshData> g_Meshes;

bool LoadFile(const char* file_path);