#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "LoadFBX.h"
#include <string>

enum class PrimitiveType
{
    CUBE,
    SPHERE,
    CYLINDER,
    PLANE,
    UNKNOWN
};

class PrimitiveGenerator
{
public:
    // Generate a cube mesh and return its index in g_Meshes
    static int GenerateCube(float size = 1.0f);

    // Generate a sphere mesh and return its index in g_Meshes
    static int GenerateSphere(float radius = 1.0f, int segments = 32, int rings = 16);

    // Generate a cylinder mesh and return its index in g_Meshes
    static int GenerateCylinder(float radius = 0.5f, float height = 2.0f, int segments = 32);

    // Generate a plane mesh and return its index in g_Meshes
    static int GeneratePlane(float width = 1.0f, float depth = 1.0f, int widthSegments = 1, int depthSegments = 1);

    // Get mesh path for a primitive type
    static std::string GetPrimitiveMeshPath(PrimitiveType type, float param1 = 1.0f, float param2 = 1.0f, int param3 = 32, int param4 = 16);

    // Save primitive mesh to file
    static bool SavePrimitiveMesh(PrimitiveType type, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, float param1, float param2, int param3, int param4);

    // Load primitive mesh from file
    static int LoadPrimitiveMesh(const std::string& primitivePath);

private:
    // Helper function to create and upload mesh data to GPU
    static int CreateMeshData(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);

    // Create mesh data and save to file
    static int CreateAndSaveMeshData(PrimitiveType type, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, float param1, float param2, int param3, int param4);
};