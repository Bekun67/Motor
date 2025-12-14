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

    //aabb variables
    glm::vec3 aabbMin = glm::vec3(0.0f);
    glm::vec3 aabbMax = glm::vec3(0.0f);

    //variables to normalize scale
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 1.0f;
    glm::vec3 minBound = glm::vec3(0.0f);
    glm::vec3 maxBound = glm::vec3(0.0f);
};

struct MeshWithTransform 
{
    int meshIndex;
    int originalMeshIndex;
    glm::mat4 transform;
};

extern std::vector<MeshWithTransform> g_MeshInstances;

bool LoadFile(const char* file_path);

extern glm::vec3 g_ModelCenter;
extern float g_ModelRadius;
extern std::vector<MeshData> g_Meshes;

// Load using custom file format (fast)
bool LoadFileCustomFormat(const char* file_path);

// Import, Save and Load workflow
bool ImportSaveLoad(const char* file_path);

void DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);