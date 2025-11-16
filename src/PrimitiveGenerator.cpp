#include "PrimitiveGenerator.h"
#include "MeshImporter.h"
#include "FileSystemManager.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::string PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType type, float param1, float param2, int param3, int param4)
{
    std::stringstream ss;
    ss << "Library/Meshes/Primitives/";

    switch (type)
    {
    case PrimitiveType::CUBE:
        ss << "Cube_" << std::fixed << std::setprecision(2) << param1 << ".ilmesh";
        break;
    case PrimitiveType::SPHERE:
        ss << "Sphere_r" << std::fixed << std::setprecision(2) << param1
            << "_seg" << param3 << "_rings" << param4 << ".ilmesh";
        break;
    case PrimitiveType::CYLINDER:
        ss << "Cylinder_r" << std::fixed << std::setprecision(2) << param1
            << "_h" << std::fixed << std::setprecision(2) << param2
            << "_seg" << param3 << ".ilmesh";
        break;
    case PrimitiveType::PLANE:
        ss << "Plane_w" << std::fixed << std::setprecision(2) << param1
            << "_d" << std::fixed << std::setprecision(2) << param2
            << "_wseg" << param3 << "_dseg" << param4 << ".ilmesh";
        break;
    default:
        ss << "Unknown.ilmesh";
        break;
    }

    return ss.str();
}

bool PrimitiveGenerator::SavePrimitiveMesh(PrimitiveType type, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, float param1, float param2, int param3, int param4)
{
    // Create primitives directory if it doesn't exist
    std::string primitivesDir = "Library/Meshes/Primitives/";
    if (!std::filesystem::exists(primitivesDir))
    {
        std::filesystem::create_directories(primitivesDir);
    }

    std::string path = GetPrimitiveMeshPath(type, param1, param2, param3, param4);

    // Check if already exists
    if (FileSystemManager::FileExists(path))
    {
        std::cout << "[PrimitiveGenerator] Primitive mesh already exists: " << path << std::endl;
        return true;
    }

    // Convert to CustomMesh format
    CustomMesh customMesh;
    customMesh.vertices = vertices;
    customMesh.indices = indices;
    customMesh.numVertices = vertices.size() / 8; // 8 floats per vertex
    customMesh.numIndices = indices.size();

    // Calculate AABB
    customMesh.aabbMinX = std::numeric_limits<float>::max();
    customMesh.aabbMinY = std::numeric_limits<float>::max();
    customMesh.aabbMinZ = std::numeric_limits<float>::max();
    customMesh.aabbMaxX = std::numeric_limits<float>::lowest();
    customMesh.aabbMaxY = std::numeric_limits<float>::lowest();
    customMesh.aabbMaxZ = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < vertices.size(); i += 8)
    {
        float x = vertices[i];
        float y = vertices[i + 1];
        float z = vertices[i + 2];

        customMesh.aabbMinX = std::min(customMesh.aabbMinX, x);
        customMesh.aabbMinY = std::min(customMesh.aabbMinY, y);
        customMesh.aabbMinZ = std::min(customMesh.aabbMinZ, z);

        customMesh.aabbMaxX = std::max(customMesh.aabbMaxX, x);
        customMesh.aabbMaxY = std::max(customMesh.aabbMaxY, y);
        customMesh.aabbMaxZ = std::max(customMesh.aabbMaxZ, z);
    }

    // Save using MeshImporter
    if (MeshImporter::SaveMesh(customMesh, path))
    {
        std::cout << "[PrimitiveGenerator] Primitive mesh saved: " << path << std::endl;
        return true;
    }

    std::cerr << "[PrimitiveGenerator] Failed to save primitive mesh: " << path << std::endl;
    return false;
}

int PrimitiveGenerator::LoadPrimitiveMesh(const std::string& primitivePath)
{
    CustomMesh mesh;
    if (!MeshImporter::LoadMesh(mesh, primitivePath))
    {
        std::cerr << "[PrimitiveGenerator] Failed to load primitive mesh: " << primitivePath << std::endl;
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

    std::cout << "[PrimitiveGenerator] Primitive mesh loaded to engine at index: " << meshIndex << std::endl;
    return meshIndex;
}

int PrimitiveGenerator::CreateMeshData(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
{
    MeshData md;

    glm::vec3 minBound(FLT_MAX);
    glm::vec3 maxBound(-FLT_MAX);

    for (size_t i = 0; i < vertices.size(); i += 8) {
        glm::vec3 pos(vertices[i], vertices[i + 1], vertices[i + 2]);

        minBound.x = std::min(minBound.x, pos.x);
        minBound.y = std::min(minBound.y, pos.y);
        minBound.z = std::min(minBound.z, pos.z);

        maxBound.x = std::max(maxBound.x, pos.x);
        maxBound.y = std::max(maxBound.y, pos.y);
        maxBound.z = std::max(maxBound.z, pos.z);
    }

    md.aabbMin = minBound;
    md.aabbMax = maxBound;

    // Calculate center and radius for normalization
    md.center = (minBound + maxBound) * 0.5f;
    md.radius = glm::length(maxBound - md.center);
    md.minBound = minBound;
    md.maxBound = maxBound;

    // Create VAO and VBO
    glGenVertexArrays(1, &md.VAO);
    glBindVertexArray(md.VAO);

    glGenBuffers(1, &md.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Create EBO
    glGenBuffers(1, &md.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, md.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    GLsizei vertexSize = 8 * sizeof(float);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(0));

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));

    // UV attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(6 * sizeof(float)));

    md.numIndices = static_cast<GLsizei>(indices.size());

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Add mesh to global list and return its index
    int meshIndex = (int)g_Meshes.size();
    g_Meshes.push_back(md);

    std::cout << "Created primitive mesh -> VAO " << md.VAO
        << " VBO " << md.VBO
        << " EBO " << md.EBO
        << " indices " << md.numIndices
        << " (meshIndex: " << meshIndex << ")" << std::endl;

    return meshIndex;
}

int PrimitiveGenerator::CreateAndSaveMeshData(PrimitiveType type, const std::vector<float>& vertices, const std::vector<unsigned int>& indices, float param1, float param2, int param3, int param4)
{
    // Save to file
    SavePrimitiveMesh(type, vertices, indices, param1, param2, param3, param4);

    // Create and return engine index
    return CreateMeshData(vertices, indices);
}

int PrimitiveGenerator::GenerateCube(float size)
{
    float halfSize = size * 0.5f;

    // Vertices
    std::vector<float> vertices = {
        // Front face (Z+)
        -halfSize, -halfSize,  halfSize,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         halfSize, -halfSize,  halfSize,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         halfSize,  halfSize,  halfSize,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -halfSize,  halfSize,  halfSize,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,

        // Back face (Z-)
         halfSize, -halfSize, -halfSize,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
        -halfSize, -halfSize, -halfSize,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
        -halfSize,  halfSize, -halfSize,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
         halfSize,  halfSize, -halfSize,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,

         // Right face (X+)
          halfSize, -halfSize,  halfSize,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
          halfSize, -halfSize, -halfSize,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
          halfSize,  halfSize, -halfSize,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
          halfSize,  halfSize,  halfSize,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

          // Left face (X-)
          -halfSize, -halfSize, -halfSize,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
          -halfSize, -halfSize,  halfSize,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
          -halfSize,  halfSize,  halfSize,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
          -halfSize,  halfSize, -halfSize,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

          // Top face (Y+)
          -halfSize,  halfSize,  halfSize,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
           halfSize,  halfSize,  halfSize,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
           halfSize,  halfSize, -halfSize,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
          -halfSize,  halfSize, -halfSize,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,

          // Bottom face (Y-)
          -halfSize, -halfSize, -halfSize,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
           halfSize, -halfSize, -halfSize,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
           halfSize, -halfSize,  halfSize,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
          -halfSize, -halfSize,  halfSize,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
    };

    std::vector<unsigned int> indices = {
        0,  1,  2,   0,  2,  3,   // Front
        4,  5,  6,   4,  6,  7,   // Back
        8,  9,  10,  8,  10, 11,  // Right
        12, 13, 14,  12, 14, 15,  // Left
        16, 17, 18,  16, 18, 19,  // Top
        20, 21, 22,  20, 22, 23   // Bottom
    };

    return CreateAndSaveMeshData(PrimitiveType::CUBE, vertices, indices, size, 0, 0, 0);
}

int PrimitiveGenerator::GenerateSphere(float radius, int segments, int rings)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (int ring = 0; ring <= rings; ++ring)
    {
        float phi = M_PI * ring / rings;
        float y = radius * cos(phi);
        float ringRadius = radius * sin(phi);

        for (int seg = 0; seg <= segments; ++seg)
        {
            float theta = 2.0f * M_PI * seg / segments;
            float x = ringRadius * cos(theta);
            float z = ringRadius * sin(theta);

            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Normal normalized position for a sphere centered at origin
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // UV coordinates
            float u = (float)seg / segments;
            float v = (float)ring / rings;
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    // Generate indices
    for (int ring = 0; ring < rings; ++ring)
    {
        for (int seg = 0; seg < segments; ++seg)
        {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;

            // Two triangles per quad
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    return CreateAndSaveMeshData(PrimitiveType::SPHERE, vertices, indices, radius, 0, segments, rings);
}

int PrimitiveGenerator::GenerateCylinder(float radius, float height, int segments)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float halfHeight = height * 0.5f;

    // Generate side vertices
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * M_PI * i / segments;
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        // Normal for side (perpendicular to cylinder axis)
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        // Top vertex
        vertices.push_back(x);
        vertices.push_back(halfHeight);
        vertices.push_back(z);
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);
        vertices.push_back((float)i / segments);
        vertices.push_back(1.0f);

        // Bottom vertex
        vertices.push_back(x);
        vertices.push_back(-halfHeight);
        vertices.push_back(z);
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);
        vertices.push_back((float)i / segments);
        vertices.push_back(0.0f);
    }

    // Generate side indices
    int sideVertexCount = (segments + 1) * 2;
    for (int i = 0; i < segments; ++i)
    {
        int topCurrent = i * 2;
        int bottomCurrent = topCurrent + 1;
        int topNext = (i + 1) * 2;
        int bottomNext = topNext + 1;

        indices.push_back(topCurrent);
        indices.push_back(bottomCurrent);
        indices.push_back(topNext);

        indices.push_back(topNext);
        indices.push_back(bottomCurrent);
        indices.push_back(bottomNext);
    }

    // Top cap center vertex
    int topCenterIndex = sideVertexCount;
    vertices.push_back(0.0f);
    vertices.push_back(halfHeight);
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    vertices.push_back(1.0f);
    vertices.push_back(0.0f);
    vertices.push_back(0.5f);
    vertices.push_back(0.5f);

    // Top cap rim vertices
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * M_PI * i / segments;
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        vertices.push_back(x);
        vertices.push_back(halfHeight);
        vertices.push_back(z);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.5f + 0.5f * cos(theta));
        vertices.push_back(0.5f + 0.5f * sin(theta));
    }

    // Top cap indices
    for (int i = 0; i < segments; ++i)
    {
        indices.push_back(topCenterIndex);
        indices.push_back(topCenterIndex + i + 1);
        indices.push_back(topCenterIndex + i + 2);
    }

    // Bottom cap center vertex
    int bottomCenterIndex = topCenterIndex + segments + 2;
    vertices.push_back(0.0f);
    vertices.push_back(-halfHeight);
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    vertices.push_back(-1.0f);
    vertices.push_back(0.0f);
    vertices.push_back(0.5f);
    vertices.push_back(0.5f);

    // Bottom cap rim vertices
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * M_PI * i / segments;
        float x = radius * cos(theta);
        float z = radius * sin(theta);

        vertices.push_back(x);
        vertices.push_back(-halfHeight);
        vertices.push_back(z);
        vertices.push_back(0.0f);
        vertices.push_back(-1.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.5f + 0.5f * cos(theta));
        vertices.push_back(0.5f + 0.5f * sin(theta));
    }

    // Bottom cap indices (reversed winding for correct normal)
    for (int i = 0; i < segments; ++i)
    {
        indices.push_back(bottomCenterIndex);
        indices.push_back(bottomCenterIndex + i + 2);
        indices.push_back(bottomCenterIndex + i + 1);
    }

    return CreateAndSaveMeshData(PrimitiveType::CYLINDER, vertices, indices, radius, height, segments, 0);
}

int PrimitiveGenerator::GeneratePlane(float width, float depth, int widthSegments, int depthSegments)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float halfWidth = width * 0.5f;
    float halfDepth = depth * 0.5f;

    // Generate vertices
    for (int z = 0; z <= depthSegments; ++z)
    {
        float zPos = -halfDepth + (depth * z / depthSegments);
        float v = (float)z / depthSegments;

        for (int x = 0; x <= widthSegments; ++x)
        {
            float xPos = -halfWidth + (width * x / widthSegments);
            float u = (float)x / widthSegments;

            // Position
            vertices.push_back(xPos);
            vertices.push_back(0.0f);
            vertices.push_back(zPos);

            // Normal (pointing up)
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);

            // UV
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    // Generate indices
    for (int z = 0; z < depthSegments; ++z)
    {
        for (int x = 0; x < widthSegments; ++x)
        {
            int topLeft = z * (widthSegments + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (widthSegments + 1) + x;
            int bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    return CreateAndSaveMeshData(PrimitiveType::PLANE, vertices, indices, width, depth, widthSegments, depthSegments);
}