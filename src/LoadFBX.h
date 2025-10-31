#pragma once
#include <glad/glad.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>

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

bool LoadFile(const char* file_path);

extern glm::vec3 g_ModelCenter;
extern float g_ModelRadius;
extern std::vector<MeshData> g_Meshes;