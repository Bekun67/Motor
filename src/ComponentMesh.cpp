#include "ComponentMesh.h"
#include "Application.h"
#include "ComponentTransform.h"
#include "ComponentTexture.h"
#include "GameObject.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

ComponentMesh::ComponentMesh(GameObject* gameObject)
    : Component(gameObject, ComponentType::MESH),
    meshdata(nullptr),
    drawOutline(false)
{
}

ComponentMesh::~ComponentMesh()
{
    meshdata = nullptr;
}

void ComponentMesh::Update()
{
    // Update logic if needed
}

void ComponentMesh::Draw(Camera* camera)
{
    if (meshdata == nullptr || meshdata->VAO == 0 || meshdata->numIndices == 0)
        return;

    // Get shader program from OpenGL module
    unsigned int shaderProgram = Application::GetInstance().opengl->shaderProgram;
    glUseProgram(shaderProgram);

    // Get transform component
    ComponentTransform* transform = gameObject->transform;
    if (transform == nullptr)
        return;

    // Build model matrix from transform
    glm::mat4 model = glm::mat4(1.0f);

    // Apply translation
    model = glm::translate(model, glm::vec3(
        transform->translation.x,
        transform->translation.y,
        transform->translation.z
    ));

    // Apply rotation (convert quaternion to matrix)
    glm::quat quat(
        transform->rotation.w,
        transform->rotation.x,
        transform->rotation.y,
        transform->rotation.z
    );
    model *= glm::mat4_cast(quat);

    // Apply scale
    model = glm::scale(model, glm::vec3(
        transform->scaling.x,
        transform->scaling.y,
        transform->scaling.z
    ));

    // Get view and projection matrices
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    // Set uniforms
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model_matrix");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Bind texture if available
    ComponentTexture* texComp = gameObject->texture;
    if (texComp != nullptr && texComp->hasTexture && texComp->texturedata != nullptr)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texComp->texturedata->id);
        GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(texLoc, 0);
    }
    else if (!meshdata->textures.empty())
    {
        // Use mesh's own texture if available
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, meshdata->textures[0].id);
        GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(texLoc, 0);
    }

    // Draw the mesh
    glBindVertexArray(meshdata->VAO);
    glDrawElements(GL_TRIANGLES, meshdata->numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Draw outline if enabled
    if (drawOutline)
    {
        // TODO: Implement outline drawing using stencil buffer
    }
}