#include "ResourceManager.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "FileSystemManager.h"
#include "LoadFBX.h"
#include "ModuleEditor.h"
#include "PrimitiveGenerator.h"
#include <filesystem>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace fs = std::filesystem;

std::map<std::string, MeshResource> ResourceManager::s_MeshCache;

bool ResourceManager::EnsureMeshExists(const std::string& fbxPath, int meshIndexInFBX, int& outEngineIndex)
{
    if (fbxPath.empty())
    {
        LOG_WARNING("Empty FBX path provided to EnsureMeshExists");
        return false;
    }

    // Check if this is a primitive mesh
    if (fbxPath.find("Library/Meshes/Primitives/") == 0)
    {
        // This is a primitive, load it directly
        outEngineIndex = PrimitiveGenerator::LoadPrimitiveMesh(fbxPath);
        if (outEngineIndex >= 0)
        {
            LOG("Primitive mesh loaded: " + fbxPath);
            return true;
        }
        else
        {
            LOG_ERROR("Failed to load primitive mesh: " + fbxPath);
            return false;
        }
    }

    // Generate custom mesh path (for FBX models)
    std::string customMeshPath = MeshImporter::GetCustomMeshPath(fbxPath, meshIndexInFBX);

    // Check if custom mesh already exists
    if (FileSystemManager::FileExists(customMeshPath))
    {
        // Load it to engine and get index
        outEngineIndex = LoadMeshToEngine(customMeshPath);
        if (outEngineIndex >= 0)
        {
            LOG("Mesh loaded from custom format: " + customMeshPath);
            return true;
        }
    }

    // Custom mesh doesn't exist, try to import from FBX
    if (!FileSystemManager::FileExists(fbxPath))
    {
        // Try to find FBX in Assets folder
        std::string fbxName = fs::path(fbxPath).filename().string();
        std::string foundPath = FindFBXInAssets(fbxName);

        if (foundPath.empty())
        {
            LOG_ERROR("FBX file not found: " + fbxPath);
            return false;
        }

        // Use found path
        LOG("Found FBX in Assets: " + foundPath);
        if (!ImportAndSaveMesh(foundPath, meshIndexInFBX))
        {
            LOG_ERROR("Failed to import mesh from: " + foundPath);
            return false;
        }

        // Update custom mesh path with found path
        customMeshPath = MeshImporter::GetCustomMeshPath(foundPath, meshIndexInFBX);
    }
    else
    {
        // Import from original FBX path
        if (!ImportAndSaveMesh(fbxPath, meshIndexInFBX))
        {
            LOG_ERROR("Failed to import mesh from: " + fbxPath);
            return false;
        }
    }

    // Load newly created custom mesh
    outEngineIndex = LoadMeshToEngine(customMeshPath);
    if (outEngineIndex >= 0)
    {
        LOG("Mesh imported and loaded successfully: " + customMeshPath);
        return true;
    }

    LOG_ERROR("Failed to load mesh after import: " + customMeshPath);
    return false;
}

bool ResourceManager::EnsureTextureExists(const std::string& texturePath)
{
    if (texturePath.empty() || texturePath == "checkerboard")
    {
        return true; // Checkerboard doesn't need to exist
    }

    // Generate custom texture path
    std::string customTexturePath = TextureImporter::GetCustomTexturePath(texturePath);

    // Check if custom texture already exists
    if (FileSystemManager::FileExists(customTexturePath))
    {
        LOG("Texture exists in custom format: " + customTexturePath);
        return true;
    }

    // Custom texture doesn't exist, try to import
    if (!FileSystemManager::FileExists(texturePath))
    {
        // Try to find texture in Assets folder
        std::string textureName = fs::path(texturePath).filename().string();
        std::string foundPath = FindTextureInAssets(textureName);

        if (foundPath.empty())
        {
            LOG_WARNING("Texture file not found: " + texturePath);
            return false;
        }

        LOG("Found texture in Assets: " + foundPath);
        return ImportAndSaveTexture(foundPath);
    }
    else
    {
        // Import from original texture path
        return ImportAndSaveTexture(texturePath);
    }
}

std::string ResourceManager::FindFBXInAssets(const std::string& fbxName)
{
    std::string assetsDir = "Assets/Models/";

    if (!fs::exists(assetsDir))
    {
        return "";
    }

    // Search recursively in Assets/Models
    for (const auto& entry : fs::recursive_directory_iterator(assetsDir))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            std::string extension = entry.path().extension().string();

            // Convert to lowercase for comparison
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".fbx" && filename == fbxName)
            {
                return entry.path().string();
            }
        }
    }

    // Also search in root Assets folder
    std::string rootAssetsDir = "Assets/";
    if (fs::exists(rootAssetsDir))
    {
        for (const auto& entry : fs::recursive_directory_iterator(rootAssetsDir))
        {
            if (entry.is_regular_file())
            {
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();

                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if (extension == ".fbx" && filename == fbxName)
                {
                    return entry.path().string();
                }
            }
        }
    }

    return "";
}

std::string ResourceManager::FindTextureInAssets(const std::string& textureName)
{
    std::string assetsDir = "Assets/Textures/";

    if (!fs::exists(assetsDir))
    {
        return "";
    }

    // Search in Assets/Textures
    for (const auto& entry : fs::recursive_directory_iterator(assetsDir))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            std::string extension = entry.path().extension().string();

            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if ((extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                extension == ".tga" || extension == ".dds") && filename == textureName)
            {
                return entry.path().string();
            }
        }
    }

    // Also search in root Assets folder
    std::string rootAssetsDir = "Assets/";
    if (fs::exists(rootAssetsDir))
    {
        for (const auto& entry : fs::recursive_directory_iterator(rootAssetsDir))
        {
            if (entry.is_regular_file())
            {
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();

                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if ((extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                    extension == ".tga" || extension == ".dds") && filename == textureName)
                {
                    return entry.path().string();
                }
            }
        }
    }

    return "";
}

bool ResourceManager::ImportAndSaveMesh(const std::string& fbxPath, int meshIndex)
{
    LOG("Importing mesh from FBX: " + fbxPath + " (index: " + std::to_string(meshIndex) + ")");

    // Import all meshes from FBX
    std::vector<CustomMesh> meshes = MeshImporter::ImportFBX(fbxPath);

    if (meshes.empty() || meshIndex >= (int)meshes.size())
    {
        LOG_ERROR("Mesh index out of range or no meshes found in FBX");
        return false;
    }

    // Save the specific mesh
    std::string customMeshPath = MeshImporter::GetCustomMeshPath(fbxPath, meshIndex);

    if (!MeshImporter::SaveMesh(meshes[meshIndex], customMeshPath))
    {
        LOG_ERROR("Failed to save mesh to: " + customMeshPath);
        return false;
    }

    LOG("Mesh saved successfully to: " + customMeshPath);
    return true;
}

bool ResourceManager::ImportAndSaveTexture(const std::string& texturePath)
{
    LOG("Importing texture: " + texturePath);

    CustomTexture texture = TextureImporter::ImportTexture(texturePath);

    if (texture.width == 0 || texture.height == 0)
    {
        LOG_ERROR("Failed to import texture: " + texturePath);
        return false;
    }

    std::string customTexturePath = TextureImporter::GetCustomTexturePath(texturePath);

    if (!TextureImporter::SaveTexture(texture, customTexturePath))
    {
        LOG_ERROR("Failed to save texture to: " + customTexturePath);
        return false;
    }

    LOG("Texture saved successfully to: " + customTexturePath);
    return true;
}

int ResourceManager::LoadMeshToEngine(const std::string& customMeshPath)
{
    CustomMesh mesh;
    if (!MeshImporter::LoadMesh(mesh, customMeshPath))
    {
        LOG_ERROR("Failed to load custom mesh: " + customMeshPath);
        return -1;
    }

    // Create MeshData and upload to GPU
    MeshData md;

    md.aabbMin = glm::vec3(mesh.aabbMinX, mesh.aabbMinY, mesh.aabbMinZ);
    md.aabbMax = glm::vec3(mesh.aabbMaxX, mesh.aabbMaxY, mesh.aabbMaxZ);

    // Create VAO and VBO
    glGenVertexArrays(1, &md.VAO);
    glBindVertexArray(md.VAO);

    glGenBuffers(1, &md.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float),
        mesh.vertices.data(), GL_STATIC_DRAW);

    // Create EBO
    glGenBuffers(1, &md.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, md.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int),
        mesh.indices.data(), GL_STATIC_DRAW);

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

    md.numIndices = mesh.numIndices;

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Add to global mesh list
    int meshIndex = (int)g_Meshes.size();
    g_Meshes.push_back(md);

    LOG("Mesh loaded to engine at index: " + std::to_string(meshIndex));
    return meshIndex;
}

int ResourceManager::GetMeshIndexByPath(const std::string& fbxPath, int meshIndexInFBX)
{
    std::string key = fbxPath + "_" + std::to_string(meshIndexInFBX);

    if (s_MeshCache.count(key))
    {
        return s_MeshCache[key].meshIndexInEngine;
    }

    return -1;
}