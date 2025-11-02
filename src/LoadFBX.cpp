#include "LoadFBX.h"
#include "ConsoleWindow.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <cstdio>
#include <vector>
#include <cstring>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>

#define LOG(format, ...) \
    do { \
        printf(format "\n", __VA_ARGS__); \
        char buffer[512]; \
        snprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
        ConsoleWindow::AddLog(buffer, LogType::INFO); \
    } while(0)

#define LOG_ERROR(format, ...) \
    do { \
        printf(format "\n", __VA_ARGS__); \
        char buffer[512]; \
        snprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
        ConsoleWindow::AddLog(buffer, LogType::ERROR_LOG); \
    } while(0)

std::vector<MeshData> g_Meshes;
glm::vec3 g_ModelCenter(0.0f);
float g_ModelRadius = 1.0f;

bool LoadFile(const char* file_path) {
    LOG("Loading model: %s", file_path);

    Assimp::Importer importer;

    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_FlipUVs |
        aiProcess_GlobalScale |
        aiProcess_PreTransformVertices;

    const aiScene* scene = importer.ReadFile(file_path, flags);

    if (!scene) {
        LOG_ERROR("Assimp error: %s", importer.GetErrorString());
        return false;
    }

    if (!scene->HasMeshes()) {
        LOG_ERROR("No meshes found in file: %s", file_path);
        return false;
    }

    LOG("Found %d meshes in file", scene->mNumMeshes);

    glm::vec3 minBound(FLT_MAX);
    glm::vec3 maxBound(-FLT_MAX);

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];
        LOG("Processing mesh %d: %d vertices, %d faces", m, mesh->mNumVertices, mesh->mNumFaces);

        std::vector<float> vertexData;
        std::vector<uint32_t> indices;

        vertexData.reserve(mesh->mNumVertices * 8);
        indices.reserve(mesh->mNumFaces * 3);

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D& pos = mesh->mVertices[v];
            minBound.x = std::min(minBound.x, pos.x);
            minBound.y = std::min(minBound.y, pos.y);
            minBound.z = std::min(minBound.z, pos.z);
            maxBound.x = std::max(maxBound.x, pos.x);
            maxBound.y = std::max(maxBound.y, pos.y);
            maxBound.z = std::max(maxBound.z, pos.z);

            vertexData.push_back(pos.x);
            vertexData.push_back(pos.y);
            vertexData.push_back(pos.z);

            aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.0f, 0.0f, 1.0f);
            vertexData.push_back(normal.x);
            vertexData.push_back(normal.y);
            vertexData.push_back(normal.z);

            if (mesh->HasTextureCoords(0)) {
                aiVector3D uv = mesh->mTextureCoords[0][v];
                vertexData.push_back(uv.x);
                vertexData.push_back(uv.y);
            }
            else {
                vertexData.push_back(0.0);
                vertexData.push_back(0.0);
            }
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                LOG_ERROR("ERROR: face with not 3 indices detected (mesh %d face %d)", m, f);
                continue;
            }
            else {
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }
        }

        MeshData md;

        glGenVertexArrays(1, &md.VAO);
        glBindVertexArray(md.VAO);

        glGenBuffers(1, &md.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &md.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, md.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        GLsizei vertexSize = 8 * sizeof(float);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(0));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(6 * sizeof(float)));

        md.numIndices = static_cast<GLsizei>(indices.size());

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        g_Meshes.push_back(md);
        LOG("Loaded mesh %d -> VAO %u VBO %u EBO %u indices %d", m, md.VAO, md.VBO, md.EBO, md.numIndices);
    }

    g_ModelCenter = (minBound + maxBound) * 0.5f;
    g_ModelRadius = glm::length(maxBound - g_ModelCenter);

    LOG("Model loaded successfully. Center: (%.2f, %.2f, %.2f), Radius: %.2f",
        g_ModelCenter.x, g_ModelCenter.y, g_ModelCenter.z, g_ModelRadius);

    return true;
}