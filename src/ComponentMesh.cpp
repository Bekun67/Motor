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
    meshIndex(-1)
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
    if (texComp != nullptr && texComp->hasTexture && texComp->texturedata != nullptr)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texComp->texturedata->id);
        GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(texLoc, 0);
    }
    else if (!meshdata.textures.empty())
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, meshdata.textures[0].id);
        GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(texLoc, 0);
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    //draw mesh
    glBindVertexArray(meshdata.VAO);
    glDrawElements(GL_TRIANGLES, meshdata.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}