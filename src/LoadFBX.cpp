#include "LoadFBX.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <cstdio>
#include <vector>
#include <cstring>
#include <iostream>

#define LOG(format, ...) printf(format "\n", __VA_ARGS__)

// Definici?n del vector global
std::vector<MeshData> g_Meshes;

bool LoadFile(const char* file_path) {
    Assimp::Importer importer;

    // Postprocess flags: triangulate, generate normals if missing, join identical vertices, flip UVs if needed
    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality | aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(file_path, flags);

    if (!scene) {
        LOG("Assimp error: %s", importer.GetErrorString());
        return false;
    }

    if (!scene->HasMeshes()) {
        LOG("No meshes found in file: %s", file_path);
        return false;
    }

    // Para cada malla del scene, construimos buffers OpenGL
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];

        // Arrays temporales en CPU
        std::vector<float> vertexData;         // interleaved: pos.x,pos.y,pos.z, normal.x,normal.y,normal.z, tex.u,tex.v
        std::vector<uint32_t> indices;

        // Reserva aproximada
        vertexData.reserve(mesh->mNumVertices * 8);
        indices.reserve(mesh->mNumFaces * 3);

        // Copia de v?rtices
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            // Position
            const aiVector3D& pos = mesh->mVertices[v];
            vertexData.push_back(pos.x);
            vertexData.push_back(pos.y);
            vertexData.push_back(pos.z);

            // Normal (si no hay, el postprocess aiProcess_GenSmoothNormals la habr? generado)
            aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.0f, 0.0f, 1.0f);
            vertexData.push_back(normal.x);
            vertexData.push_back(normal.y);
            vertexData.push_back(normal.z);

            // TexCoords (canal 0)
            if (mesh->HasTextureCoords(0)) {
                aiVector3D uv = mesh->mTextureCoords[0][v];
                vertexData.push_back(uv.x);
                vertexData.push_back(uv.y);
            }
            else {
                // Si no hay UVs, a?adir 0,0
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
            }
        }

        // Copia de ?ndices (asumimos triangulos porque usamos aiProcess_Triangulate)
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                LOG("WARNING: face with != 3 indices detected (mesh %d face %d). Skipping.", m, f);
                continue;
            }
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        // Crear VAO/VBO/EBO en GPU
        MeshData md;
        glGenVertexArrays(1, &md.VAO);
        glBindVertexArray(md.VAO);

        glGenBuffers(1, &md.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &md.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, md.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        // Formato interleaved: 3 pos, 3 normal, 2 uv = stride 8 floats
        GLsizei stride = 8 * sizeof(float);

        // position -> layout(location = 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)(0));

        // normal -> layout(location = 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

        // texcoord -> layout(location = 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        // Guardar n?mero de ?ndices
        md.numIndices = static_cast<GLsizei>(indices.size());

        // Unbind por higiene
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        g_Meshes.push_back(md);

        LOG("Loaded mesh %d -> VAO %u VBO %u EBO %u indices %d", m, md.VAO, md.VBO, md.EBO, md.numIndices);
    }

    return true;
}