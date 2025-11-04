#include "PrimitiveGenerator.h"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int PrimitiveGenerator::CreateMeshData(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
{
    MeshData md;

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

    return CreateMeshData(vertices, indices);
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

    return CreateMeshData(vertices, indices);
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

    return CreateMeshData(vertices, indices);
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

    return CreateMeshData(vertices, indices);
}