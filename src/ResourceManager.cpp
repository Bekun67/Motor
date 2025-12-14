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
    if (s_MeshCache.count(customMeshPath) > 0)
    {
        int cachedIndex = s_MeshCache[customMeshPath].meshIndexInEngine;
        if (cachedIndex >= 0 && cachedIndex < (int)g_Meshes.size())
        {
            LOG("Mesh already in engine at index: " + std::to_string(cachedIndex));

            // ✅ NUEVO: Exportar el mesh desde g_Meshes (con modificaciones) al archivo .ilmesh
            if (!FileSystemManager::FileExists(customMeshPath))
            {
                LOG("Exporting modified mesh from engine to: " + customMeshPath);
                if (!ExportMeshFromEngine(cachedIndex, customMeshPath))
                {
                    LOG_ERROR("Failed to export mesh from engine");
                    return false;
                }
            }

            outEngineIndex = cachedIndex;
            return true;
        }
    }

    // Check if custom mesh file already exists
    if (FileSystemManager::FileExists(customMeshPath))
    {
        // Load newly created custom mesh
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

    outEngineIndex = LoadMeshToEngine(customMeshPath);
    if (outEngineIndex >= 0)
    {
        LOG("Mesh imported and loaded successfully: " + customMeshPath);
        return true;
    }

    LOG_ERROR("Failed to load mesh after import: " + customMeshPath);
    return false;
}

bool ResourceManager::ExportMeshFromEngine(int engineMeshIndex, const std::string& outputPath)
{
    if (engineMeshIndex < 0 || engineMeshIndex >= (int)g_Meshes.size())
    {
        LOG_ERROR("Invalid mesh index for export: " + std::to_string(engineMeshIndex));
        return false;
    }

    const MeshData& meshData = g_Meshes[engineMeshIndex];

    glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
    GLint vboSize;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vboSize);

    std::vector<float> vertices(vboSize / sizeof(float));
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, vboSize, vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.EBO);
    GLint eboSize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);

    std::vector<unsigned int> indices(eboSize / sizeof(unsigned int));
    glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboSize, indices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    CustomMesh customMesh;
    customMesh.vertices = vertices;
    customMesh.indices = indices;
    customMesh.numIndices = meshData.numIndices;

    customMesh.aabbMinX = meshData.aabbMin.x;
    customMesh.aabbMinY = meshData.aabbMin.y;
    customMesh.aabbMinZ = meshData.aabbMin.z;
    customMesh.aabbMaxX = meshData.aabbMax.x;
    customMesh.aabbMaxY = meshData.aabbMax.y;
    customMesh.aabbMaxZ = meshData.aabbMax.z;

    customMesh.centerX = meshData.center.x;
    customMesh.centerY = meshData.center.y;
    customMesh.centerZ = meshData.center.z;

    if (!MeshImporter::SaveMesh(customMesh, outputPath))
    {
        LOG_ERROR("Failed to save exported mesh to: " + outputPath);
        return false;
    }

    LOG("Successfully exported mesh to: " + outputPath);
    return true;
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
    static std::map<std::string, bool> s_ImportedFBXs;

    if (s_ImportedFBXs.count(fbxPath) == 0)
    {
        LOG("Importing ALL meshes from FBX: " + fbxPath);

        std::vector<CustomMesh> meshes = MeshImporter::ImportFBX(fbxPath);

        if (meshes.empty())
        {
            LOG_ERROR("No meshes found in FBX");
            return false;
        }

        for (size_t i = 0; i < meshes.size(); ++i)
        {
            std::string customMeshPath = MeshImporter::GetCustomMeshPath(fbxPath, i);

            if (!MeshImporter::SaveMesh(meshes[i], customMeshPath))
            {
                LOG_ERROR("Failed to save mesh " + std::to_string(i) + " to: " + customMeshPath);
                return false;
            }
        }

        s_ImportedFBXs[fbxPath] = true;

        LOG("Successfully imported and saved " + std::to_string(meshes.size()) + " meshes from: " + fbxPath);
    }
    else
    {
        std::string customMeshPath = MeshImporter::GetCustomMeshPath(fbxPath, meshIndex);
        if (!FileSystemManager::FileExists(customMeshPath))
        {
            LOG_ERROR("Mesh file doesn't exist after import: " + customMeshPath);
            return false;
        }
    }

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
    if (s_MeshCache.count(customMeshPath) > 0)
    {
        int cachedIndex = s_MeshCache[customMeshPath].meshIndexInEngine;
        if (cachedIndex >= 0 && cachedIndex < (int)g_Meshes.size())
        {
            LOG("Mesh already loaded at index: " + std::to_string(cachedIndex));
            return cachedIndex;
        }
    }

    CustomMesh mesh;
    if (!MeshImporter::LoadMesh(mesh, customMeshPath))
    {
        LOG_ERROR("Failed to load custom mesh: " + customMeshPath);
        return -1;
    }

    MeshData md;

    md.aabbMin = glm::vec3(mesh.aabbMinX, mesh.aabbMinY, mesh.aabbMinZ);
    md.aabbMax = glm::vec3(mesh.aabbMaxX, mesh.aabbMaxY, mesh.aabbMaxZ);

    //center
    md.center = glm::vec3(mesh.centerX, mesh.centerY, mesh.centerZ);

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

    MeshResource resource;
    resource.meshIndexInEngine = meshIndex;
    resource.customPath = customMeshPath;
    s_MeshCache[customMeshPath] = resource;

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