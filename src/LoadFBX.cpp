#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include "LoadFBX.h"
#include <glad/glad.h>

#define LOG(format, ...) printf(format "\n", __VA_ARGS__)

struct VertexData {
    // Index buffer data
    uint32_t id_index = 0;   
    uint32_t num_index = 0;  
    uint32_t* index = nullptr; 

    // Vertex buffer data
    uint32_t id_vertex = 0;   
    uint32_t num_vertex = 0;  
    float* vertex = nullptr;  

    // Destructor 
    ~VertexData() {
        delete[] index;
        delete[] vertex;
    }
};

// Define a structure for ourMesh
struct MeshData {
    unsigned int num_vertices = 0;
    float* vertices = nullptr;
    unsigned int num_indices = 0;
    unsigned int* indices = nullptr;

    // Destructor to clean up dynamically allocated memory
    ~MeshData() {
        delete[] vertices;
        delete[] indices;
    }
};


MeshData ourMesh;

bool LoadFile(const char* file_path) {
    // Import the file with Assimp
    const aiScene* scene = aiImportFile(file_path, aiProcessPreset_TargetRealtime_MaxQuality);

    if (scene != nullptr && scene->HasMeshes()) {
        // Iterate through the meshes
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[i];

            // Crear un nuevo VertexData para esta malla
            VertexData vertexData;

            // Copiar vértices
            vertexData.num_vertex = mesh->mNumVertices;
            vertexData.vertex = new float[vertexData.num_vertex * 3];
            memcpy(vertexData.vertex, mesh->mVertices, sizeof(float) * vertexData.num_vertex * 3);

            // Crear un buffer de vértices en la VRAM
            glGenBuffers(1, &vertexData.id_vertex);
            glBindBuffer(GL_ARRAY_BUFFER, vertexData.id_vertex);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexData.num_vertex * 3, vertexData.vertex, GL_STATIC_DRAW);

            LOG("New vertex buffer created with ID %d and %d vertices", vertexData.id_vertex, vertexData.num_vertex);

            // Copiar índices si la malla tiene caras
            if (mesh->HasFaces()) {
                vertexData.num_index = mesh->mNumFaces * 3; // Asumimos que cada cara es un triángulo
                vertexData.index = new uint32_t[vertexData.num_index];
                for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
                    if (mesh->mFaces[j].mNumIndices != 3) {
                        LOG("WARNING, geometry face with != 3 indices!");
                    }
                    else {
                        memcpy(&vertexData.index[j * 3], mesh->mFaces[j].mIndices, 3 * sizeof(uint32_t));
                    }
                }

                // Crear un buffer de índices en la VRAM
                glGenBuffers(1, &vertexData.id_index);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexData.id_index);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * vertexData.num_index, vertexData.index, GL_STATIC_DRAW);

                LOG("New index buffer created with ID %d and %d indices", vertexData.id_index, vertexData.num_index);
            }
        }

        //for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        //    aiMaterial* material = scene->mMaterials[i];
        //        
        //    
        //}

        aiReleaseImport(scene);
        return true; // Successfully loaded the file
    }
    else {
        // Log an error message
        LOG("Error loading file: %s", file_path);
        return false; // Failed to load the file
    }
}
