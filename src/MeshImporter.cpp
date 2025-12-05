#include "MeshImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <glm/glm.hpp>

namespace fs = std::filesystem;

std::vector<CustomMesh> MeshImporter::ImportFBX(const std::string& fbxPath)
{
    std::vector<CustomMesh> meshes;

    Assimp::Importer importer;
    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_ImproveCacheLocality |
        aiProcess_FlipUVs |
        aiProcess_GlobalScale |
        aiProcess_PreTransformVertices;

    const aiScene* scene = importer.ReadFile(fbxPath, flags);

    if (!scene || !scene->HasMeshes())
    {
        std::cerr << "Failed to import FBX: " << fbxPath << std::endl;
        return meshes;
    }

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
    {
        aiMesh* mesh = scene->mMeshes[m];
        CustomMesh customMesh;

        glm::vec3 meshMin(FLT_MAX);
        glm::vec3 meshMax(-FLT_MAX);

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
        {
            const aiVector3D& pos = mesh->mVertices[v];
            meshMin.x = std::min(meshMin.x, pos.x);
            meshMin.y = std::min(meshMin.y, pos.y);
            meshMin.z = std::min(meshMin.z, pos.z);
            meshMax.x = std::max(meshMax.x, pos.x);
            meshMax.y = std::max(meshMax.y, pos.y);
            meshMax.z = std::max(meshMax.z, pos.z);
        }

        glm::vec3 meshCenter = (meshMin + meshMax) * 0.5f;

        customMesh.centerX = meshCenter.x;
        customMesh.centerY = meshCenter.y;
        customMesh.centerZ = meshCenter.z;

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
        {
            const aiVector3D& pos = mesh->mVertices[v];

            customMesh.vertices.push_back(pos.x - meshCenter.x);
            customMesh.vertices.push_back(pos.y - meshCenter.y);
            customMesh.vertices.push_back(pos.z - meshCenter.z);

            aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.0f, 0.0f, 1.0f);
            customMesh.vertices.push_back(normal.x);
            customMesh.vertices.push_back(normal.y);
            customMesh.vertices.push_back(normal.z);

            if (mesh->HasTextureCoords(0))
            {
                aiVector3D uv = mesh->mTextureCoords[0][v];
                customMesh.vertices.push_back(uv.x);
                customMesh.vertices.push_back(uv.y);
            }
            else
            {
                customMesh.vertices.push_back(0.0f);
                customMesh.vertices.push_back(0.0f);
            }
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices == 3)
            {
                customMesh.indices.push_back(face.mIndices[0]);
                customMesh.indices.push_back(face.mIndices[1]);
                customMesh.indices.push_back(face.mIndices[2]);
            }
        }

        customMesh.numIndices = customMesh.indices.size();

        customMesh.aabbMinX = meshMin.x - meshCenter.x;
        customMesh.aabbMinY = meshMin.y - meshCenter.y;
        customMesh.aabbMinZ = meshMin.z - meshCenter.z;
        customMesh.aabbMaxX = meshMax.x - meshCenter.x;
        customMesh.aabbMaxY = meshMax.y - meshCenter.y;
        customMesh.aabbMaxZ = meshMax.z - meshCenter.z;

        meshes.push_back(customMesh);
    }

    return meshes;
}

bool MeshImporter::SaveMesh(const CustomMesh& mesh, const std::string& filepath)
{
    fs::path filePath(filepath);
    fs::create_directories(filePath.parent_path());

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to save mesh: " << filepath << std::endl;
        return false;
    }

    uint32_t magic = 0x4D455348; 
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

    uint32_t version = 2; 
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    uint32_t vertexCount = mesh.vertices.size();
    file.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
    file.write(reinterpret_cast<const char*>(mesh.vertices.data()), vertexCount * sizeof(float));

    uint32_t indexCount = mesh.indices.size();
    file.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));
    file.write(reinterpret_cast<const char*>(mesh.indices.data()), indexCount * sizeof(unsigned int));

    file.write(reinterpret_cast<const char*>(&mesh.numIndices), sizeof(mesh.numIndices));

    file.write(reinterpret_cast<const char*>(&mesh.aabbMinX), sizeof(float));
    file.write(reinterpret_cast<const char*>(&mesh.aabbMinY), sizeof(float));
    file.write(reinterpret_cast<const char*>(&mesh.aabbMinZ), sizeof(float));
    file.write(reinterpret_cast<const char*>(&mesh.aabbMaxX), sizeof(float));
    file.write(reinterpret_cast<const char*>(&mesh.aabbMaxY), sizeof(float));
    file.write(reinterpret_cast<const char*>(&mesh.aabbMaxZ), sizeof(float));

    file.write(reinterpret_cast<const char*>(&mesh.centerX), sizeof(float));
    file.write(reinterpret_cast<const char*>(&mesh.centerY), sizeof(float));
    file.write(reinterpret_cast<const char*>(&mesh.centerZ), sizeof(float));

    file.close();
    return true;
}

bool MeshImporter::LoadMesh(CustomMesh& mesh, const std::string& filepath)
{
    auto start = std::chrono::high_resolution_clock::now();

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to load mesh: " << filepath << std::endl;
        return false;
    }

    // Magic number
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x4D455348)
    {
        std::cerr << "Invalid mesh file format" << std::endl;
        return false;
    }

    // Version
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    // Vertices
    uint32_t vertexCount;
    file.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
    mesh.vertices.resize(vertexCount);
    file.read(reinterpret_cast<char*>(mesh.vertices.data()), vertexCount * sizeof(float));

    // Indices
    uint32_t indexCount;
    file.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
    mesh.indices.resize(indexCount);
    file.read(reinterpret_cast<char*>(mesh.indices.data()), indexCount * sizeof(unsigned int));

    // Num indices
    file.read(reinterpret_cast<char*>(&mesh.numIndices), sizeof(mesh.numIndices));

    // AABB
    file.read(reinterpret_cast<char*>(&mesh.aabbMinX), sizeof(float));
    file.read(reinterpret_cast<char*>(&mesh.aabbMinY), sizeof(float));
    file.read(reinterpret_cast<char*>(&mesh.aabbMinZ), sizeof(float));
    file.read(reinterpret_cast<char*>(&mesh.aabbMaxX), sizeof(float));
    file.read(reinterpret_cast<char*>(&mesh.aabbMaxY), sizeof(float));
    file.read(reinterpret_cast<char*>(&mesh.aabbMaxZ), sizeof(float));

    if (version >= 2)
    {
        file.read(reinterpret_cast<char*>(&mesh.centerX), sizeof(float));
        file.read(reinterpret_cast<char*>(&mesh.centerY), sizeof(float));
        file.read(reinterpret_cast<char*>(&mesh.centerZ), sizeof(float));
    }
    else
    {
        mesh.centerX = (mesh.aabbMinX + mesh.aabbMaxX) * 0.5f;
        mesh.centerY = (mesh.aabbMinY + mesh.aabbMaxY) * 0.5f;
        mesh.centerZ = (mesh.aabbMinZ + mesh.aabbMaxZ) * 0.5f;
    }

    file.close();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "[MeshImporter] Custom mesh loaded in " << duration.count() << " ms" << std::endl;
    std::cout << "[MeshImporter] " << (vertexCount / 8) << " vertices, " << indexCount << " indices" << std::endl;

    return true;
}

std::string MeshImporter::GetCustomMeshPath(const std::string& fbxPath, int meshIndex)
{
    fs::path path(fbxPath);
    std::string baseName = path.stem().string();
    std::string customPath = "Library/Meshes/" + baseName + "_" + std::to_string(meshIndex) + ".ilmesh";
    return customPath;
}