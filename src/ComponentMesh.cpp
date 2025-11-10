#include "ComponentMesh.h"
#include "Application.h"
#include "ComponentTransform.h"
#include "ComponentTexture.h"
#include "GameObject.h"
#include "LoadFBX.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

ComponentMesh::ComponentMesh(GameObject* gameObject)
    : Component(gameObject, ComponentType::MESH),
    meshIndex(-1),
    showVertexNormals(false),
    showFaceNormals(false)
{
}

ComponentMesh::~ComponentMesh()
{
    meshIndex = -1;
}

void ComponentMesh::Update()
{
}

void ComponentMesh::Draw(Camera* camera)
{
    if (meshIndex < 0 || meshIndex >= (int)g_Meshes.size())
    {
        //if index is not valid
        return;
    }

    //get the desired mesh using the given index
    MeshData& meshdata = g_Meshes[meshIndex];

    if (meshdata.VAO == 0 || meshdata.numIndices == 0)
    {
        return;
    }

    unsigned int shaderProgram = Application::GetInstance().opengl->shaderProgram;
    glUseProgram(shaderProgram);

    //get transform
    ComponentTransform* transform = gameObject->transform;
    if (transform == nullptr)
    {
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);

    //apply translate to the mesh
    model = glm::translate(model, glm::vec3(
        transform->translation.x,
        transform->translation.y,
        transform->translation.z
    ));

    //apply rotation
    glm::quat quat(
        transform->rotation.w,
        transform->rotation.x,
        transform->rotation.y,
        transform->rotation.z
    );
    model *= glm::mat4_cast(quat);

    //lastly, apply scale
    model = glm::scale(model, glm::vec3(
        transform->scaling.x,
        transform->scaling.y,
        transform->scaling.z
    ));

    //get the matrices
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model_matrix");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //bind texture
    ComponentTexture* texComp = gameObject->texture;
    bool texturebound = false;

    //try to use the texture of component texture
    if (texComp != nullptr && texComp->hasTexture && texComp->texturedata != nullptr)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texComp->texturedata->id);
        GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(texLoc, 0);
        texturebound = true;
    }
    //if no texture available try to use the fbx one
    else if (!meshdata.textures.empty())
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, meshdata.textures[0].id);
        GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(texLoc, 0);
        texturebound = true;
    }
    //if no texture at all unbind (ge would use the checkerboard)
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    //draw mesh
    glBindVertexArray(meshdata.VAO);
    glDrawElements(GL_TRIANGLES, meshdata.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    if (texturebound) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Draw normals when true
    if (showVertexNormals)
    {
        DrawVertexNormals(camera);
    }

    if (showFaceNormals)
    {
        DrawFaceNormals(camera);
    }
}

void ComponentMesh::DrawVertexNormals(Camera* camera, float length)
{
    if (meshIndex < 0 || meshIndex >= (int)g_Meshes.size())
    {
        return;
    }

    MeshData& meshdata = g_Meshes[meshIndex];
    ComponentTransform* transform = gameObject->transform;
    if (transform == nullptr)
    {
        return;
    }

    // Transformation matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        transform->translation.x,
        transform->translation.y,
        transform->translation.z
    ));

    glm::quat quat(
        transform->rotation.w,
        transform->rotation.x,
        transform->rotation.y,
        transform->rotation.z
    );
    model *= glm::mat4_cast(quat);

    model = glm::scale(model, glm::vec3(
        transform->scaling.x,
        transform->scaling.y,
        transform->scaling.z
    ));

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    // Read VBO
    glBindBuffer(GL_ARRAY_BUFFER, meshdata.VBO);
    GLint bufferSize;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

    int vertexSize = 8; 
    int numVertices = bufferSize / (vertexSize * sizeof(float));

    std::vector<float> vertexData(bufferSize / sizeof(float));
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertexData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // create lines
    std::vector<float> normalLines;
    for (int i = 0; i < numVertices; ++i)
    {
        int offset = i * vertexSize;

        glm::vec3 pos(vertexData[offset], vertexData[offset + 1], vertexData[offset + 2]);
        glm::vec3 normal(vertexData[offset + 3], vertexData[offset + 4], vertexData[offset + 5]);


        glm::vec4 worldPos = model * glm::vec4(pos, 1.0f);


        glm::vec3 worldNormal = glm::normalize(normalMatrix * normal);


        normalLines.push_back(worldPos.x);
        normalLines.push_back(worldPos.y);
        normalLines.push_back(worldPos.z);


        glm::vec3 endPoint = glm::vec3(worldPos) + worldNormal * length;
        normalLines.push_back(endPoint.x);
        normalLines.push_back(endPoint.y);
        normalLines.push_back(endPoint.z);
    }

    // temporal VAO and VBO
    GLuint normalVAO, normalVBO;
    glGenVertexArrays(1, &normalVAO);
    glGenBuffers(1, &normalVBO);

    glBindVertexArray(normalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normalLines.size() * sizeof(float), normalLines.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // use normal shaders
    unsigned int normalShader = Application::GetInstance().opengl->normalShaderProgram;
    glUseProgram(normalShader);

    glm::mat4 identityModel = glm::mat4(1.0f);
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    GLint modelLoc = glGetUniformLocation(normalShader, "model_matrix");
    GLint viewLoc = glGetUniformLocation(normalShader, "view");
    GLint projLoc = glGetUniformLocation(normalShader, "projection");
    GLint colorLoc = glGetUniformLocation(normalShader, "lineColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identityModel));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // blue
    glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);

    glDrawArrays(GL_LINES, 0, normalLines.size() / 3);

    glBindVertexArray(0);
    glDeleteBuffers(1, &normalVBO);
    glDeleteVertexArrays(1, &normalVAO);
}

void ComponentMesh::DrawFaceNormals(Camera* camera, float length)
{
    if (meshIndex < 0 || meshIndex >= (int)g_Meshes.size())
    {
        return;
    }

    MeshData& meshdata = g_Meshes[meshIndex];
    ComponentTransform* transform = gameObject->transform;
    if (transform == nullptr)
    {
        return;
    }

    // transformation matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        transform->translation.x,
        transform->translation.y,
        transform->translation.z
    ));

    glm::quat quat(
        transform->rotation.w,
        transform->rotation.x,
        transform->rotation.y,
        transform->rotation.z
    );
    model *= glm::mat4_cast(quat);

    model = glm::scale(model, glm::vec3(
        transform->scaling.x,
        transform->scaling.y,
        transform->scaling.z
    ));

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, meshdata.VBO);
    GLint vboSize;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vboSize);

    std::vector<float> vertexData(vboSize / sizeof(float));
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, vboSize, vertexData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshdata.EBO);
    GLint eboSize;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);

    std::vector<unsigned int> indices(eboSize / sizeof(unsigned int));
    glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboSize, indices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // create lines
    std::vector<float> faceNormalLines;
    int vertexSize = 8; 

    for (size_t i = 0; i < indices.size(); i += 3)
    {

        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        int offset0 = idx0 * vertexSize;
        int offset1 = idx1 * vertexSize;
        int offset2 = idx2 * vertexSize;

        glm::vec3 v0(vertexData[offset0], vertexData[offset0 + 1], vertexData[offset0 + 2]);
        glm::vec3 v1(vertexData[offset1], vertexData[offset1 + 1], vertexData[offset1 + 2]);
        glm::vec3 v2(vertexData[offset2], vertexData[offset2 + 1], vertexData[offset2 + 2]);

        // calculate face center
        glm::vec3 faceCenter = (v0 + v1 + v2) / 3.0f;


        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

 
        glm::vec4 worldCenter = model * glm::vec4(faceCenter, 1.0f);
        glm::vec3 worldNormal = glm::normalize(normalMatrix * faceNormal);


        faceNormalLines.push_back(worldCenter.x);
        faceNormalLines.push_back(worldCenter.y);
        faceNormalLines.push_back(worldCenter.z);

        glm::vec3 endPoint = glm::vec3(worldCenter) + worldNormal * length;
        faceNormalLines.push_back(endPoint.x);
        faceNormalLines.push_back(endPoint.y);
        faceNormalLines.push_back(endPoint.z);
    }

    // temporal VAO and VBO
    GLuint faceNormalVAO, faceNormalVBO;
    glGenVertexArrays(1, &faceNormalVAO);
    glGenBuffers(1, &faceNormalVBO);

    glBindVertexArray(faceNormalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, faceNormalVBO);
    glBufferData(GL_ARRAY_BUFFER, faceNormalLines.size() * sizeof(float), faceNormalLines.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // use normal shaders
    unsigned int normalShader = Application::GetInstance().opengl->normalShaderProgram;
    glUseProgram(normalShader);

    glm::mat4 identityModel = glm::mat4(1.0f);
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    GLint modelLoc = glGetUniformLocation(normalShader, "model_matrix");
    GLint viewLoc = glGetUniformLocation(normalShader, "view");
    GLint projLoc = glGetUniformLocation(normalShader, "projection");
    GLint colorLoc = glGetUniformLocation(normalShader, "lineColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identityModel));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // red
    glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);

    glDrawArrays(GL_LINES, 0, faceNormalLines.size() / 3);

    glBindVertexArray(0);
    glDeleteBuffers(1, &faceNormalVBO);
    glDeleteVertexArrays(1, &faceNormalVAO);
}