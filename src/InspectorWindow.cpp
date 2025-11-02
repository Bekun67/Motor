#include "InspectorWindow.h"
#include "HierarchyWindow.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

InspectorWindow::InspectorWindow()
    : EditorWindow("Inspector", true),
    showNormals(false),
    showCheckerTexture(false)
{
}

InspectorWindow::~InspectorWindow()
{
}

void InspectorWindow::Draw()
{
    if (!ImGui::Begin(name.c_str(), &visible))
    {
        ImGui::End();
        return;
    }

    GameObject* selected = HierarchyWindow::GetSelectedGameObject();

    if (selected == nullptr)
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No GameObject selected");
        ImGui::End();
        return;
    }

    // GameObject name
    ImGui::Text("Name: %s", selected->name.c_str());
    ImGui::Separator();

    // Draw components
    DrawTransformComponent(selected);
    DrawMeshComponent(selected);
    DrawTextureComponent(selected);

    ImGui::End();
}

void InspectorWindow::DrawTransformComponent(GameObject* go)
{
    if (!go || !go->transform) return;

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ComponentTransform* transform = go->transform;

        // Position
        ImGui::Text("Position:");
        float pos[3] = { transform->translation.x, transform->translation.y, transform->translation.z };
        if (ImGui::DragFloat3("##Position", pos, 0.1f))
        {
            transform->translation.x = pos[0];
            transform->translation.y = pos[1];
            transform->translation.z = pos[2];
        }

        // Rotation (convert quaternion to euler angles for display)
        ImGui::Text("Rotation:");
        glm::quat quat(transform->rotation.w, transform->rotation.x,
            transform->rotation.y, transform->rotation.z);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(quat));
        float rot[3] = { euler.x, euler.y, euler.z };
        if (ImGui::DragFloat3("##Rotation", rot, 1.0f))
        {
            // Convert back to quaternion
            glm::vec3 radians = glm::radians(glm::vec3(rot[0], rot[1], rot[2]));
            glm::quat newQuat = glm::quat(radians);
            transform->rotation.w = newQuat.w;
            transform->rotation.x = newQuat.x;
            transform->rotation.y = newQuat.y;
            transform->rotation.z = newQuat.z;
        }

        // Scale
        ImGui::Text("Scale:");
        float scale[3] = { transform->scaling.x, transform->scaling.y, transform->scaling.z };
        if (ImGui::DragFloat3("##Scale", scale, 0.01f, 0.001f, 100.0f))
        {
            transform->scaling.x = scale[0];
            transform->scaling.y = scale[1];
            transform->scaling.z = scale[2];
        }
    }
}

void InspectorWindow::DrawMeshComponent(GameObject* go)
{
    if (!go || !go->mesh) return;

    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ComponentMesh* mesh = go->mesh;

        if (mesh->meshdata)
        {
            ImGui::Text("Vertices: %d", mesh->meshdata->numIndices);
            ImGui::Text("VAO: %u", mesh->meshdata->VAO);
            ImGui::Text("VBO: %u", mesh->meshdata->VBO);
            ImGui::Text("EBO: %u", mesh->meshdata->EBO);

            ImGui::Separator();

            if (ImGui::Checkbox("Show Normals", &showNormals))
            {
                // TODO: Implement normal visualization
            }

            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(TODO)");

            if (ImGui::Checkbox("Draw Outline", &mesh->drawOutline))
            {
                // Outline drawing handled in ComponentMesh::Draw
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No mesh data");
        }
    }
}

void InspectorWindow::DrawTextureComponent(GameObject* go)
{
    if (!go || !go->texture) return;

    if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ComponentTexture* texture = go->texture;

        if (texture->hasTexture && texture->texturedata)
        {
            ImGui::Text("Path: %s", texture->texturePath.c_str());
            ImGui::Text("Texture ID: %u", texture->texturedata->id);

            // Display texture preview
            ImGui::Separator();
            ImGui::Text("Preview:");

            // Get texture size info
            GLuint texID = texture->texturedata->id;
            glBindTexture(GL_TEXTURE_2D, texID);
            GLint width, height;
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
            glBindTexture(GL_TEXTURE_2D, 0);

            ImGui::Text("Size: %dx%d", width, height);

            // Display texture - Note: UV coordinates are (0,0) top-left, (1,1) bottom-right
            ImGui::Image((ImTextureID)(intptr_t)texID,
                ImVec2(128.0f, 128.0f),
                ImVec2(0.0f, 0.0f),  // UV0
                ImVec2(1.0f, 1.0f),  // UV1
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f),  // Tint color
                ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Border color

            ImGui::Separator();

            if (ImGui::Checkbox("Show Checker Texture", &showCheckerTexture))
            {
                // TODO: Implement checker texture visualization
            }

            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(TODO)");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f), "No texture assigned");
        }
    }
}