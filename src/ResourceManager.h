#pragma once
#include <string>
#include <vector>
#include <map>
#include "GameObject.h"

struct MeshResource
{
    std::string fbxPath;
    std::string customPath;
    int meshIndexInFBX;
    int meshIndexInEngine;
};

class ResourceManager
{
public:
    // Ensure mesh is saved to custom format
    static bool EnsureMeshExists(const std::string& fbxPath, int meshIndexInFBX, int& outEngineIndex);

    // Ensure texture is saved to custom format
    static bool EnsureTextureExists(const std::string& texturePath);

    // Find FBX file in Assets folder
    static std::string FindFBXInAssets(const std::string& fbxName);

    // Find texture file in Assets folder
    static std::string FindTextureInAssets(const std::string& textureName);

    // Import and save mesh from FBX
    static bool ImportAndSaveMesh(const std::string& fbxPath, int meshIndex);

    // Import and save texture
    static bool ImportAndSaveTexture(const std::string& texturePath);

    // Load mesh from custom format and add to g_Meshes
    static int LoadMeshToEngine(const std::string& customMeshPath);

    // Get mesh index by FBX path and mesh index
    static int GetMeshIndexByPath(const std::string& fbxPath, int meshIndexInFBX);

    static bool ExportMeshFromEngine(int engineMeshIndex, const std::string& outputPath);

private:
    static std::map<std::string, MeshResource> s_MeshCache;
};