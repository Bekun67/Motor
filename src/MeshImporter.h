#pragma once
#include <string>
#include <vector>
#include <fstream>

// Forward declarations (no Assimp in header)
struct aiMesh;

// Custom mesh structure (no Assimp dependency)
struct CustomMesh {
    std::vector<float> vertices;      // Position (3) + Normal (3) + UV (2) = 8 floats per vertex
    std::vector<unsigned int> indices;

    // Metadata
    unsigned int numVertices;
    unsigned int numIndices;

    float aabbMinX, aabbMinY, aabbMinZ;
    float aabbMaxX, aabbMaxY, aabbMaxZ;

    float centerX, centerY, centerZ;

    CustomMesh() : numVertices(0), numIndices(0),
        aabbMinX(0), aabbMinY(0), aabbMinZ(0),
        aabbMaxX(0), aabbMaxY(0), aabbMaxZ(0),
        centerX(0), centerY(0), centerZ(0) {
    }
};

namespace MeshImporter {
    // Import from Assimp (only used internally in .cpp)
    CustomMesh ImportFromAssimp(const aiMesh* mesh);

    // Import FBX file and return all meshes
    std::vector<CustomMesh> ImportFBX(const std::string& fbxPath);

    // Save custom mesh to binary format
    bool SaveMesh(const CustomMesh& mesh, const std::string& outputPath);

    // Load custom mesh from binary format
    bool LoadMesh(CustomMesh& mesh, const std::string& inputPath);

    // Get the custom mesh file path from original FBX path
    std::string GetCustomMeshPath(const std::string& fbxPath, int meshIndex);
}