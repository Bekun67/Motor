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
#include <glad/glad.h>

#define LOG(format, ...) printf(format "\n", __VA_ARGS__)

std::vector<MeshData> g_Meshes;

bool LoadFile(const char* file_path) {
    Assimp::Importer importer;

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

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];

        std::vector<float> vertexData;         
        std::vector<uint32_t> indices;

        vertexData.reserve(mesh->mNumVertices * 8); //vertexs
        indices.reserve(mesh->mNumFaces * 3); //faces

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D& pos = mesh->mVertices[v];
            vertexData.push_back(pos.x);
            vertexData.push_back(pos.y);
            vertexData.push_back(pos.z);

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

        // Copia de ?ndices (asumimos triangulos porque usamos aiProcess_Triangulate)
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                LOG("ERROR: face with not 3 indices detected (mesh %i face %i)", m, f);
                continue;
            }
            else {
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }
        }

        MeshData md;

        //create vao and vbo to associate them
        glGenVertexArrays(1, &md.VAO);
        glBindVertexArray(md.VAO);

        glGenBuffers(1, &md.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, md.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        //load faces
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
        g_Meshes.push_back(md);
        LOG("Loaded mesh %i -> VAO %u VBO %u EBO %u indices %i", m, md.VAO, md.VBO, md.EBO, md.numIndices);
    }

    return true;
}