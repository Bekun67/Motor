#include "MeshImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace MeshImporter {

    // Internal function - Import single mesh from Assimp
    CustomMesh ImportFromAssimp(const aiMesh* mesh) {
        CustomMesh customMesh;

        if (!mesh) return customMesh;

        customMesh.numVertices = mesh->mNumVertices;
        customMesh.vertices.reserve(mesh->mNumVertices * 8); // 8 floats per vertex

        // Copy vertices, normals, and UVs
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            // Position
            customMesh.vertices.push_back(mesh->mVertices[i].x);
            customMesh.vertices.push_back(mesh->mVertices[i].y);
            customMesh.vertices.push_back(mesh->mVertices[i].z);

            // Normal
            if (mesh->HasNormals()) {
                customMesh.vertices.push_back(mesh->mNormals[i].x);
                customMesh.vertices.push_back(mesh->mNormals[i].y);
                customMesh.vertices.push_back(mesh->mNormals[i].z);
            }
            else {
                customMesh.vertices.push_back(0.0f);
                customMesh.vertices.push_back(1.0f);
                customMesh.vertices.push_back(0.0f);
            }

            // UV
            if (mesh->HasTextureCoords(0)) {
                customMesh.vertices.push_back(mesh->mTextureCoords[0][i].x);
                customMesh.vertices.push_back(mesh->mTextureCoords[0][i].y);
            }
            else {
                customMesh.vertices.push_back(0.0f);
                customMesh.vertices.push_back(0.0f);
            }
        }

        // Copy indices
        customMesh.numIndices = mesh->mNumFaces * 3;
        customMesh.indices.reserve(customMesh.numIndices);

        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];
            if (face.mNumIndices == 3) {
                customMesh.indices.push_back(face.mIndices[0]);
                customMesh.indices.push_back(face.mIndices[1]);
                customMesh.indices.push_back(face.mIndices[2]);
            }
        }

        return customMesh;
    }

    // Import FBX file and return all meshes
    std::vector<CustomMesh> ImportFBX(const std::string& fbxPath) {
        std::vector<CustomMesh> meshes;

        auto startTime = std::chrono::high_resolution_clock::now();

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(fbxPath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_FlipUVs |
            aiProcess_PreTransformVertices);

        if (!scene || !scene->HasMeshes()) {
            std::cerr << "[MeshImporter] Failed to load FBX: " << fbxPath << std::endl;
            if (!scene) {
                std::cerr << "[MeshImporter] Assimp error: " << importer.GetErrorString() << std::endl;
            }
            return meshes;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "[MeshImporter] FBX loaded in " << duration.count() << " ms" << std::endl;
        std::cout << "[MeshImporter] Found " << scene->mNumMeshes << " meshes" << std::endl;

        // Convert all meshes
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            CustomMesh customMesh = ImportFromAssimp(scene->mMeshes[i]);
            meshes.push_back(customMesh);
            std::cout << "[MeshImporter] Mesh " << i << ": "
                << customMesh.numVertices << " vertices, "
                << customMesh.numIndices << " indices" << std::endl;
        }

        return meshes;
    }

    // Save custom mesh to binary format
    bool SaveMesh(const CustomMesh& mesh, const std::string& outputPath) {
        std::ofstream file(outputPath, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "[MeshImporter] Failed to open file for writing: " << outputPath << std::endl;
            return false;
        }

        // Header: numVertices, numIndices
        file.write(reinterpret_cast<const char*>(&mesh.numVertices), sizeof(unsigned int));
        file.write(reinterpret_cast<const char*>(&mesh.numIndices), sizeof(unsigned int));

        // Vertex data
        size_t vertexDataSize = mesh.vertices.size() * sizeof(float);
        file.write(reinterpret_cast<const char*>(mesh.vertices.data()), vertexDataSize);

        // Index data
        size_t indexDataSize = mesh.indices.size() * sizeof(unsigned int);
        file.write(reinterpret_cast<const char*>(mesh.indices.data()), indexDataSize);

        file.close();

        std::cout << "[MeshImporter] Saved mesh to: " << outputPath << std::endl;
        return true;
    }

    // Load custom mesh from binary format
    bool LoadMesh(CustomMesh& mesh, const std::string& inputPath) {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::ifstream file(inputPath, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "[MeshImporter] Failed to open file for reading: " << inputPath << std::endl;
            return false;
        }

        // Read header
        file.read(reinterpret_cast<char*>(&mesh.numVertices), sizeof(unsigned int));
        file.read(reinterpret_cast<char*>(&mesh.numIndices), sizeof(unsigned int));

        // Read vertex data
        size_t vertexDataSize = mesh.numVertices * 8; // 8 floats per vertex
        mesh.vertices.resize(vertexDataSize);
        file.read(reinterpret_cast<char*>(mesh.vertices.data()), vertexDataSize * sizeof(float));

        // Read index data
        mesh.indices.resize(mesh.numIndices);
        file.read(reinterpret_cast<char*>(mesh.indices.data()), mesh.numIndices * sizeof(unsigned int));

        file.close();

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "[MeshImporter] Custom mesh loaded in " << duration.count() << " ms" << std::endl;
        std::cout << "[MeshImporter] " << mesh.numVertices << " vertices, "
            << mesh.numIndices << " indices" << std::endl;

        return true;
    }

    // Get the custom mesh file path
    std::string GetCustomMeshPath(const std::string& fbxPath, int meshIndex) {
        fs::path p(fbxPath);
        std::string filename = p.stem().string();

        std::stringstream ss;
        ss << "Library/Meshes/" << filename << "_" << meshIndex << ".ilmesh";
        return ss.str();
    }
}