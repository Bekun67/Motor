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
    bool isTransparent = false;

    //try to use the texture of component texture
    if (texComp != nullptr && texComp->hasTexture && texComp->texturedata != nullptr)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texComp->texturedata->id);
        GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(texLoc, 0);
        texturebound = true;
        isTransparent = texComp->hasTransparency;
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

    if (showAABB)
    {
        DrawAABB(camera);
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

void ComponentMesh::DrawAABB(Camera* camera)
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

    //get transformation matrix
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

    //get the 8 corners of the aabb
    glm::vec3 corners[8] = {
        glm::vec3(meshdata.aabbMin.x, meshdata.aabbMin.y, meshdata.aabbMin.z),
        glm::vec3(meshdata.aabbMax.x, meshdata.aabbMin.y, meshdata.aabbMin.z),
        glm::vec3(meshdata.aabbMax.x, meshdata.aabbMax.y, meshdata.aabbMin.z),
        glm::vec3(meshdata.aabbMin.x, meshdata.aabbMax.y, meshdata.aabbMin.z),
        glm::vec3(meshdata.aabbMin.x, meshdata.aabbMin.y, meshdata.aabbMax.z),
        glm::vec3(meshdata.aabbMax.x, meshdata.aabbMin.y, meshdata.aabbMax.z),
        glm::vec3(meshdata.aabbMax.x, meshdata.aabbMax.y, meshdata.aabbMax.z),
        glm::vec3(meshdata.aabbMin.x, meshdata.aabbMax.y, meshdata.aabbMax.z)
    };

    //tramsform
    glm::vec3 worldCorners[8];
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 worldPos = model * glm::vec4(corners[i], 1.0f);
        worldCorners[i] = glm::vec3(worldPos);
    }

    //create 12 lines
    std::vector<float> lineData;

    //bottom face
    lineData.insert(lineData.end(), { worldCorners[0].x, worldCorners[0].y, worldCorners[0].z });
    lineData.insert(lineData.end(), { worldCorners[1].x, worldCorners[1].y, worldCorners[1].z });

    lineData.insert(lineData.end(), { worldCorners[1].x, worldCorners[1].y, worldCorners[1].z });
    lineData.insert(lineData.end(), { worldCorners[2].x, worldCorners[2].y, worldCorners[2].z });

    lineData.insert(lineData.end(), { worldCorners[2].x, worldCorners[2].y, worldCorners[2].z });
    lineData.insert(lineData.end(), { worldCorners[3].x, worldCorners[3].y, worldCorners[3].z });

    lineData.insert(lineData.end(), { worldCorners[3].x, worldCorners[3].y, worldCorners[3].z });
    lineData.insert(lineData.end(), { worldCorners[0].x, worldCorners[0].y, worldCorners[0].z });

    //top face
    lineData.insert(lineData.end(), { worldCorners[4].x, worldCorners[4].y, worldCorners[4].z });
    lineData.insert(lineData.end(), { worldCorners[5].x, worldCorners[5].y, worldCorners[5].z });

    lineData.insert(lineData.end(), { worldCorners[5].x, worldCorners[5].y, worldCorners[5].z });
    lineData.insert(lineData.end(), { worldCorners[6].x, worldCorners[6].y, worldCorners[6].z });

    lineData.insert(lineData.end(), { worldCorners[6].x, worldCorners[6].y, worldCorners[6].z });
    lineData.insert(lineData.end(), { worldCorners[7].x, worldCorners[7].y, worldCorners[7].z });

    lineData.insert(lineData.end(), { worldCorners[7].x, worldCorners[7].y, worldCorners[7].z });
    lineData.insert(lineData.end(), { worldCorners[4].x, worldCorners[4].y, worldCorners[4].z });

    //vertical edges
    lineData.insert(lineData.end(), { worldCorners[0].x, worldCorners[0].y, worldCorners[0].z });
    lineData.insert(lineData.end(), { worldCorners[4].x, worldCorners[4].y, worldCorners[4].z });

    lineData.insert(lineData.end(), { worldCorners[1].x, worldCorners[1].y, worldCorners[1].z });
    lineData.insert(lineData.end(), { worldCorners[5].x, worldCorners[5].y, worldCorners[5].z });

    lineData.insert(lineData.end(), { worldCorners[2].x, worldCorners[2].y, worldCorners[2].z });
    lineData.insert(lineData.end(), { worldCorners[6].x, worldCorners[6].y, worldCorners[6].z });

    lineData.insert(lineData.end(), { worldCorners[3].x, worldCorners[3].y, worldCorners[3].z });
    lineData.insert(lineData.end(), { worldCorners[7].x, worldCorners[7].y, worldCorners[7].z });

    //vao / vbo
    GLuint aabbVAO, aabbVBO;
    glGenVertexArrays(1, &aabbVAO);
    glGenBuffers(1, &aabbVBO);

    glBindVertexArray(aabbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, aabbVBO);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //normal shader
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

    //green
    glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f);

    glDrawArrays(GL_LINES, 0, lineData.size() / 3);

    glBindVertexArray(0);
    glDeleteBuffers(1, &aabbVBO);
    glDeleteVertexArrays(1, &aabbVAO);
}

PropertyMap ComponentMesh::Serialize() const
{
    PropertyMap props;
    props["meshIndex"] = meshIndex;
    props["showVertexNormals"] = showVertexNormals;
    props["showFaceNormals"] = showFaceNormals;
    props["showAABB"] = showAABB;
    return props;
}

void ComponentMesh::Deserialize(const PropertyMap& props)
{
    if (props.count("meshIndex")) meshIndex = std::get<int>(props.at("meshIndex"));
    if (props.count("showVertexNormals")) showVertexNormals = std::get<bool>(props.at("showVertexNormals"));
    if (props.count("showFaceNormals")) showFaceNormals = std::get<bool>(props.at("showFaceNormals"));
    if (props.count("showAABB")) showAABB = std::get<bool>(props.at("showAABB"));
}

WorldAABB ComponentMesh::GetWorldAABB() const
{
    WorldAABB worldAABB;

    if (meshIndex < 0 || meshIndex >= (int)g_Meshes.size())
    {
        //empty aabb if there is no mesh
        worldAABB.min = glm::vec3(0.0f);
        worldAABB.max = glm::vec3(0.0f);
        worldAABB.center = glm::vec3(0.0f);
        worldAABB.size = glm::vec3(0.0f);
        return worldAABB;
    }

    MeshData& meshdata = g_Meshes[meshIndex];
    ComponentTransform* transform = gameObject->transform;

    if (transform == nullptr)
    {
        //without transofrm component we return local aabb (the default one)
        worldAABB.min = meshdata.aabbMin;
        worldAABB.max = meshdata.aabbMax;
        worldAABB.center = (meshdata.aabbMin + meshdata.aabbMax) * 0.5f;
        worldAABB.size = meshdata.aabbMax - meshdata.aabbMin;
        return worldAABB;
    }

    //if we have transform we calculate matrix
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

    //transform all 8 corners
    glm::vec3 corners[8] = 
    {
        glm::vec3(meshdata.aabbMin.x, meshdata.aabbMin.y, meshdata.aabbMin.z),
        glm::vec3(meshdata.aabbMax.x, meshdata.aabbMin.y, meshdata.aabbMin.z),
        glm::vec3(meshdata.aabbMax.x, meshdata.aabbMax.y, meshdata.aabbMin.z),
        glm::vec3(meshdata.aabbMin.x, meshdata.aabbMax.y, meshdata.aabbMin.z),
        glm::vec3(meshdata.aabbMin.x, meshdata.aabbMin.y, meshdata.aabbMax.z),
        glm::vec3(meshdata.aabbMax.x, meshdata.aabbMin.y, meshdata.aabbMax.z),
        glm::vec3(meshdata.aabbMax.x, meshdata.aabbMax.y, meshdata.aabbMax.z),
        glm::vec3(meshdata.aabbMin.x, meshdata.aabbMax.y, meshdata.aabbMax.z)
    };

    //find new min and max
    glm::vec3 minWorld(FLT_MAX);
    glm::vec3 maxWorld(-FLT_MAX);

    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 worldPos = model * glm::vec4(corners[i], 1.0f);
        glm::vec3 worldPos3 = glm::vec3(worldPos);

        minWorld.x = std::min(minWorld.x, worldPos3.x);
        minWorld.y = std::min(minWorld.y, worldPos3.y);
        minWorld.z = std::min(minWorld.z, worldPos3.z);

        maxWorld.x = std::max(maxWorld.x, worldPos3.x);
        maxWorld.y = std::max(maxWorld.y, worldPos3.y);
        maxWorld.z = std::max(maxWorld.z, worldPos3.z);
    }

    worldAABB.min = minWorld;
    worldAABB.max = maxWorld;
    worldAABB.center = (minWorld + maxWorld) * 0.5f;
    worldAABB.size = maxWorld - minWorld;

    return worldAABB;
}

void ComponentMesh::DrawDebugRay(Camera* camera)
{
    if (!gameObject || !camera) return;

    //center of the obj
    glm::vec3 center = GetWorldAABB().center;

    //cam pos
    OpenGL* opengl = Application::GetInstance().opengl.get();
    glm::vec3 camPos = camera->GetPosition();

    //line data
    std::vector<float> lineData = {
        center.x, center.y, center.z,
        camPos.x, camPos.y - 0.1f, camPos.z
    };

    //drawing line
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int shader = Application::GetInstance().opengl->normalShaderProgram;
    glUseProgram(shader);

    glm::mat4 identity = glm::mat4(1.0f);
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    glUniformMatrix4fv(glGetUniformLocation(shader, "model_matrix"), 1, GL_FALSE, glm::value_ptr(identity));
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    if (opengl->debugZBuffer) glUniform3f(glGetUniformLocation(shader, "lineColor"), 0.5f, 0.5f, 0.5f);
    else glUniform3f(glGetUniformLocation(shader, "lineColor"), 1.0f, 0.0f, 1.0f);

    glLineWidth(2.0f);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_LINES, 0, 2);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void ComponentMesh::DrawOutline(Camera* camera, const glm::vec3& color, float thickness)
{
    if (meshIndex < 0 || meshIndex >= (int)g_Meshes.size())
    {
        return;
    }

    MeshData& meshdata = g_Meshes[meshIndex];

    if (meshdata.VAO == 0 || meshdata.numIndices == 0)
    {
        return;
    }

    ComponentTransform* transform = gameObject->transform;
    if (transform == nullptr)
    {
        return;
    }

    // Calculate model matrix
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

    // Scale slightly larger for outline
    float scaleMultiplier = 1.0f + thickness;
    model = glm::scale(model, glm::vec3(
        transform->scaling.x * scaleMultiplier,
        transform->scaling.y * scaleMultiplier,
        transform->scaling.z * scaleMultiplier
    ));

    // Get matrices
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    // Disable depth writing
    glDepthMask(GL_FALSE);

    // Draw the scaled-up mesh
    glBindVertexArray(meshdata.VAO);
    glDrawElements(GL_TRIANGLES, meshdata.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Re-enable depth writing
    glDepthMask(GL_TRUE);
}