#include "LoadFBX.h"
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
#include "MeshImporter.h"
#include "FileSystemManager.h"

#define LOG(format, ...) printf(format "\n", __VA_ARGS__)

std::vector<MeshData> g_Meshes;
glm::vec3 g_ModelCenter(0.0f);
float g_ModelRadius = 1.0f;

bool LoadFile(const char* file_path) {
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
        LOG("Assimp error: %s", importer.GetErrorString());
        return false;
    }

    if (!scene->HasMeshes()) {
        LOG("No meshes found in file: %s", file_path);
        return false;
    }

    glm::vec3 minBound(FLT_MAX);
    glm::vec3 maxBound(-FLT_MAX);

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];

        std::vector<float> vertexData;         
        std::vector<uint32_t> indices;

        vertexData.reserve(mesh->mNumVertices * 8); //vertexs
        indices.reserve(mesh->mNumFaces * 3); //faces

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
                //load uvs
                aiVector3D uv = mesh->mTextureCoords[0][v];
                vertexData.push_back(uv.x);
                vertexData.push_back(uv.y);
            }
            else {
                //if no uvs detected 
                vertexData.push_back(0.0);
                vertexData.push_back(0.0);
            }
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                LOG("ERROR: face with not 3 indices detected (mesh %i face %i)", m, f);
                continue;
            }
            else {
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }
        }

        MeshData md;

        //create vao and vbo to associate them
        glGenVertexArrays(1, &md.VAO);
        glBindVertexArray(md.VAO);

        glGenBuffers(1, &md.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        //load faces
        glGenBuffers(1, &md.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, md.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        GLsizei vertexSize = 8 * sizeof(float); //8 variables for vertex, 3 pos, 3 normals, 2 uvs

        //position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(0));

        //normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));

        //uvs
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(6 * sizeof(float)));

        md.numIndices = static_cast<GLsizei>(indices.size());

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        //add the new mesh to the list and iterate to the next one
        g_Meshes.push_back(md);
        LOG("Loaded mesh %i -> VAO %u VBO %u EBO %u indices %i", m, md.VAO, md.VBO, md.EBO, md.numIndices);
    }

    g_ModelCenter = (minBound + maxBound) * 0.5f;
    g_ModelRadius = glm::length(maxBound - g_ModelCenter);

    return true;
}

// Load using custom file format (fast)
bool LoadFileCustomFormat(const char* file_path) {
    // Determine how many meshes this FBX has by checking existing custom files
    int meshIndex = 0;
    std::vector<CustomMesh> loadedMeshes;

    while (true) {
        std::string customPath = MeshImporter::GetCustomMeshPath(file_path, meshIndex);

        if (!FileSystemManager::FileExists(customPath)) {
            break; // No more meshes
        }

        CustomMesh mesh;
        if (MeshImporter::LoadMesh(mesh, customPath)) {
            loadedMeshes.push_back(mesh);
        }
        else {
            std::cerr << "Failed to load custom mesh: " << customPath << std::endl;
            return false;
        }

        meshIndex++;
    }

    if (loadedMeshes.empty()) {
        std::cerr << "No custom meshes found for: " << file_path << std::endl;
        return false;
    }

    // Convert to OpenGL format and add to g_Meshes
    glm::vec3 minBound(FLT_MAX);
    glm::vec3 maxBound(-FLT_MAX);

    for (const auto& customMesh : loadedMeshes) {
        MeshData md;

        // Create VAO and VBO
        glGenVertexArrays(1, &md.VAO);
        glBindVertexArray(md.VAO);

        glGenBuffers(1, &md.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
        glBufferData(GL_ARRAY_BUFFER, customMesh.vertices.size() * sizeof(float),
            customMesh.vertices.data(), GL_STATIC_DRAW);

        // Create EBO
        glGenBuffers(1, &md.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, md.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, customMesh.indices.size() * sizeof(unsigned int),
            customMesh.indices.data(), GL_STATIC_DRAW);

        GLsizei vertexSize = 8 * sizeof(float);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(0));

        // Normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));

        // UVs
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(6 * sizeof(float)));

        md.numIndices = customMesh.numIndices;

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Calculate bounds
        for (size_t i = 0; i < customMesh.vertices.size(); i += 8) {
            glm::vec3 pos(customMesh.vertices[i], customMesh.vertices[i + 1], customMesh.vertices[i + 2]);
            minBound = glm::min(minBound, pos);
            maxBound = glm::max(maxBound, pos);
        }

        g_Meshes.push_back(md);
        LOG("Loaded custom mesh -> VAO %u VBO %u EBO %u indices %i",
            md.VAO, md.VBO, md.EBO, md.numIndices);
    }

    g_ModelCenter = (minBound + maxBound) * 0.5f;
    g_ModelRadius = glm::length(maxBound - g_ModelCenter);

    return true;
}

// Import, Save and Load workflow
bool ImportSaveLoad(const char* file_path) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "IMPORT -> SAVE -> LOAD Workflow" << std::endl;
    std::cout << "========================================" << std::endl;

    // Step 1: Import from FBX
    std::cout << "\n[Step 1] Importing from FBX..." << std::endl;
    auto importStart = std::chrono::high_resolution_clock::now();
    std::vector<CustomMesh> meshes = MeshImporter::ImportFBX(file_path);
    auto importEnd = std::chrono::high_resolution_clock::now();
    auto importDuration = std::chrono::duration_cast<std::chrono::milliseconds>(importEnd - importStart);

    if (meshes.empty()) {
        std::cerr << "Failed to import FBX" << std::endl;
        return false;
    }

    std::cout << "[Step 1] Import completed in " << importDuration.count() << " ms" << std::endl;

    // Step 2: Save to custom format
    std::cout << "\n[Step 2] Saving to custom format..." << std::endl;
    auto saveStart = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < meshes.size(); ++i) {
        std::string customPath = MeshImporter::GetCustomMeshPath(file_path, i);
        if (!MeshImporter::SaveMesh(meshes[i], customPath)) {
            std::cerr << "Failed to save mesh " << i << std::endl;
            return false;
        }
    }

    auto saveEnd = std::chrono::high_resolution_clock::now();
    auto saveDuration = std::chrono::duration_cast<std::chrono::milliseconds>(saveEnd - saveStart);
    std::cout << "[Step 2] Save completed in " << saveDuration.count() << " ms" << std::endl;

    // Step 3: Load from custom format
    std::cout << "\n[Step 3] Loading from custom format..." << std::endl;
    auto loadStart = std::chrono::high_resolution_clock::now();

    bool result = LoadFileCustomFormat(file_path);

    auto loadEnd = std::chrono::high_resolution_clock::now();
    auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart);

    if (result) {
        std::cout << "[Step 3] Load completed in " << loadDuration.count() << " ms" << std::endl;
    }
    else {
        std::cerr << "[Step 3] Load failed!" << std::endl;
        return false;
    }

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "SUMMARY:" << std::endl;
    std::cout << "  FBX Import time:    " << importDuration.count() << " ms" << std::endl;
    std::cout << "  Custom Save time:   " << saveDuration.count() << " ms" << std::endl;
    std::cout << "  Custom Load time:   " << loadDuration.count() << " ms" << std::endl;
    std::cout << "  Speed improvement:  " << (float)importDuration.count() / loadDuration.count() << "x faster" << std::endl;
    std::cout << "========================================\n" << std::endl;

    return true;
}