#include "LoadFBX.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstdio>
#include <vector>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include "MeshImporter.h"
#include "FileSystemManager.h"

#define LOG(format, ...) printf(format "\n", __VA_ARGS__)

std::vector<MeshData> g_Meshes;
glm::vec3 g_ModelCenter(0.0f);
float g_ModelRadius = 1.0f;
std::vector<MeshWithTransform> g_MeshInstances;

glm::mat4 aiMatrixToGlm(const aiMatrix4x4& mat) {
    return glm::mat4(
        mat.a1, mat.b1, mat.c1, mat.d1,
        mat.a2, mat.b2, mat.c2, mat.d2,
        mat.a3, mat.b3, mat.c3, mat.d3,
        mat.a4, mat.b4, mat.c4, mat.d4
    );
}

void ProcessNode(const aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) 
{
    glm::mat4 nodeTransform = aiMatrixToGlm(node->mTransformation);
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        MeshWithTransform instance;
        instance.meshIndex = node->mMeshes[i];
        instance.transform = globalTransform;
        g_MeshInstances.push_back(instance);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene, globalTransform);
    }
}

void DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale) 
{
    translation = glm::vec3(transform[3]);

    glm::vec3 row[3];
    for (int i = 0; i < 3; i++) {
        row[i] = glm::vec3(transform[i]);
    }

    scale.x = glm::length(row[0]);
    scale.y = glm::length(row[1]);
    scale.z = glm::length(row[2]);

    if (scale.x != 0) row[0] /= scale.x;
    if (scale.y != 0) row[1] /= scale.y;
    if (scale.z != 0) row[2] /= scale.z;

    glm::mat3 rotationMatrix(row[0], row[1], row[2]);
    rotation = glm::quat_cast(rotationMatrix);
}

bool LoadFile(const char* file_path) {
    Assimp::Importer importer;

    unsigned int flags = 
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_ImproveCacheLocality |
        aiProcess_FlipUVs |
        aiProcess_GlobalScale;

    const aiScene* scene = importer.ReadFile(file_path, flags);

    if (!scene) {
        LOG("Assimp error: %s", importer.GetErrorString());
        return false;
    }

    if (!scene->HasMeshes()) {
        LOG("No meshes found in file: %s", file_path);
        return false;
    }

    g_MeshInstances.clear();

    glm::vec3 minBound(FLT_MAX);
    glm::vec3 maxBound(-FLT_MAX);

    ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));

    int totalInstances = g_MeshInstances.size();
    LOG("Total instances found: %d", totalInstances);

    //get all unique meshes (will be duplicated if needed)
    std::vector<MeshData> uniqueMeshes;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];

        std::vector<float> vertexData;
        std::vector<uint32_t> indices;

        vertexData.reserve(mesh->mNumVertices * 8);
        indices.reserve(mesh->mNumFaces * 3);

        //aabb
        glm::vec3 meshMin(FLT_MAX);
        glm::vec3 meshMax(-FLT_MAX);

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D& pos = mesh->mVertices[v];
            meshMin.x = std::min(meshMin.x, pos.x);
            meshMin.y = std::min(meshMin.y, pos.y);
            meshMin.z = std::min(meshMin.z, pos.z);
            meshMax.x = std::max(meshMax.x, pos.x);
            meshMax.y = std::max(meshMax.y, pos.y);
            meshMax.z = std::max(meshMax.z, pos.z);
        }

        glm::vec3 meshCenter = (meshMin + meshMax) * 0.5f;

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D& pos = mesh->mVertices[v];

            vertexData.push_back(pos.x);
            vertexData.push_back(pos.y);
            vertexData.push_back(pos.z);

            minBound.x = std::min(minBound.x, pos.x);
            minBound.y = std::min(minBound.y, pos.y);
            minBound.z = std::min(minBound.z, pos.z);
            maxBound.x = std::max(maxBound.x, pos.x);
            maxBound.y = std::max(maxBound.y, pos.y);
            maxBound.z = std::max(maxBound.z, pos.z);

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

        //load indices
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                LOG("ERROR: face with not 3 indices detected (mesh %i face %i)", m, f);
                continue;
            }
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        MeshData md;
        //local aabb
        md.aabbMin = meshMin;
        md.aabbMax = meshMax;
        md.center = meshCenter;

        //create vao and vbo to associate them
        glGenVertexArrays(1, &md.VAO);
        glBindVertexArray(md.VAO);

        //load faces
        glGenBuffers(1, &md.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

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
        uniqueMeshes.push_back(md);

        LOG("Loaded mesh %i -> VAO %u VBO %u EBO %u indices %i", m, md.VAO, md.VBO, md.EBO, md.numIndices);
    }

    //for each instance we duplicate the mesh in g_meshes
    for (size_t i = 0; i < g_MeshInstances.size(); ++i)
    {
        MeshWithTransform& inst = g_MeshInstances[i];
        int originalMeshIndex = inst.meshIndex;

        if (originalMeshIndex >= 0 && originalMeshIndex < (int)uniqueMeshes.size())
        {
            //duplicate
            MeshData duplicatedMesh = uniqueMeshes[originalMeshIndex];

            glGenVertexArrays(1, &duplicatedMesh.VAO);
            glBindVertexArray(duplicatedMesh.VAO);

            //copy VBO
            GLint vboSize;
            glBindBuffer(GL_ARRAY_BUFFER, uniqueMeshes[originalMeshIndex].VBO);
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vboSize);

            glGenBuffers(1, &duplicatedMesh.VBO);
            glBindBuffer(GL_COPY_READ_BUFFER, uniqueMeshes[originalMeshIndex].VBO);
            glBindBuffer(GL_COPY_WRITE_BUFFER, duplicatedMesh.VBO);
            glBufferData(GL_COPY_WRITE_BUFFER, vboSize, nullptr, GL_STATIC_DRAW);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, vboSize);

            //copy EBO
            GLint eboSize;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uniqueMeshes[originalMeshIndex].EBO);
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);

            glGenBuffers(1, &duplicatedMesh.EBO);
            glBindBuffer(GL_COPY_READ_BUFFER, uniqueMeshes[originalMeshIndex].EBO);
            glBindBuffer(GL_COPY_WRITE_BUFFER, duplicatedMesh.EBO);
            glBufferData(GL_COPY_WRITE_BUFFER, eboSize, nullptr, GL_STATIC_DRAW);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, eboSize);

            //vertex attributes
            glBindVertexArray(duplicatedMesh.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, duplicatedMesh.VBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, duplicatedMesh.EBO);

            GLsizei vertexSize = 8 * sizeof(float);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(0));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(6 * sizeof(float)));

            glBindVertexArray(0);

            //add to g_meshes
            int newMeshIndex = (int)g_Meshes.size();
            g_Meshes.push_back(duplicatedMesh);
            inst.meshIndex = newMeshIndex;
        }
    }

    //clean up original unique meshes
    for (MeshData& md : uniqueMeshes) {
        if (md.EBO) glDeleteBuffers(1, &md.EBO);
        if (md.VBO) glDeleteBuffers(1, &md.VBO);
        if (md.VAO) glDeleteVertexArrays(1, &md.VAO);
    }

    g_ModelCenter = (minBound + maxBound) * 0.5f;
    g_ModelRadius = glm::length(maxBound - g_ModelCenter);

    LOG("Total mesh instances: %d", (int)g_MeshInstances.size());
    LOG("Total meshes in g_Meshes: %d", (int)g_Meshes.size());

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
            break;
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

        //aabb
        md.aabbMin = glm::vec3(customMesh.aabbMinX, customMesh.aabbMinY, customMesh.aabbMinZ);
        md.aabbMax = glm::vec3(customMesh.aabbMaxX, customMesh.aabbMaxY, customMesh.aabbMaxZ);

        //center
        md.center = (md.aabbMin + md.aabbMax) * 0.5f;

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