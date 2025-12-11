#include "InspectorWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"
#include "OpenGL.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "LoadFBX.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <map>

static char nameBuffer[128] = "";

InspectorWindow::InspectorWindow(ModuleEditor* editor)
    : EditorWindow(editor, "Inspector")
{
}

void InspectorWindow::Draw()
{
    if (!visible) return;

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (editor->firstTimeSetup)
    {
        float x = windowWidth * editor->layout.inspectorXPercent;
        float y = editor->layout.menuBarHeight + editor->layout.marginY;
        float width = windowWidth * editor->layout.inspectorWidthPercent - editor->layout.marginX;
        float height = windowHeight - editor->layout.menuBarHeight - editor->layout.marginY * 2;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (editor->useAdaptiveLayout)
    {
        float x = windowWidth * editor->layout.inspectorXPercent;
        float y = editor->layout.menuBarHeight + editor->layout.marginY;
        float width = windowWidth * editor->layout.inspectorWidthPercent - editor->layout.marginX;
        float height = windowHeight - editor->layout.menuBarHeight - editor->layout.marginY * 2;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Inspector", &visible);

    if (editor->selectedGameObjects.empty())
    {
        ImGui::Text("No GameObject selected");
    }
    else if (editor->selectedGameObjects.size() == 1)
    {
        // Show inspector for only one object
        DrawSingleObjectInspector();
    }
    else
    {
        DrawMultiObjectInspector();
    }

    ImGui::End();
}

void InspectorWindow::DrawSingleObjectInspector()
{
    GameObject* selectedGameObject = editor->selectedGameObjects[0];

    editor->textureDropPos = ImGui::GetWindowPos();
    editor->textureDropSize = ImGui::GetWindowSize();

    // editable name
    static GameObject* lastSelectedGO = nullptr;
    if (lastSelectedGO != selectedGameObject) {
        strncpy_s(nameBuffer, selectedGameObject->name.c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        lastSelectedGO = selectedGameObject;
        editor->editing = false;
    }

    ImGui::InputText("GameObject", nameBuffer, IM_ARRAYSIZE(nameBuffer));

    if (ImGui::IsItemActive()) editor->editing = true;

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editor->editing = false;
        if (strlen(nameBuffer) > 0) {
            selectedGameObject->name = std::string(nameBuffer);
        }
        else {
            strncpy_s(nameBuffer, selectedGameObject->name.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        }
    }
    ImGui::Separator();

    // Show parent information
    if (selectedGameObject->parent != nullptr)
    {
        ImGui::Text("Parent: %s", selectedGameObject->parent->name.c_str());
    }
    else
    {
        ImGui::Text("Parent: None (Root)");
    }

    // Show children count
    ImGui::Text("Children: %d", (int)selectedGameObject->children.size());

    ImGui::Separator();

    // Transform Component
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ComponentTransform* transform = selectedGameObject->transform;
        if (transform)
        {
            float pos[3] = { transform->translation.x, transform->translation.y, transform->translation.z };
            if (ImGui::DragFloat3("Position", pos, 0.1f))
            {
                transform->translation.x = pos[0];
                transform->translation.y = pos[1];
                transform->translation.z = pos[2];
                editor->sceneModified = true;
            }

            float scale[3] = { transform->scaling.x, transform->scaling.y, transform->scaling.z };
            if (ImGui::DragFloat3("Scale", scale, 0.01f, 0.01f, 100.0f))
            {
                transform->scaling.x = scale[0];
                transform->scaling.y = scale[1];
                transform->scaling.z = scale[2];
                editor->sceneModified = true;
            }

            //method to normalize angles (361º -> 1º)
            auto normalizeAngle = [](float angle) -> float
                {
                    angle = fmod(angle, 360.0f);

                    if (angle > 180.0f)
                    {
                        angle -= 360.0f;
                    }
                    else if (angle < -180.0f)
                    {
                        angle += 360.0f;
                    }

                    return angle;
                };
            //rotation
            static std::map<GameObject*, glm::vec3> rotationEulerMap;
            static std::map<GameObject*, float[3]> lastAnglesMap;

            //initialize euler angles
            if (rotationEulerMap.find(selectedGameObject) == rotationEulerMap.end()) {
                glm::quat q(transform->rotation.w, transform->rotation.x, transform->rotation.y, transform->rotation.z);
                glm::vec3 euler = glm::degrees(glm::eulerAngles(q));

                //normalized euler angles
                euler.x = normalizeAngle(euler.x);
                euler.y = normalizeAngle(euler.y);
                euler.z = normalizeAngle(euler.z);

                //apply
                rotationEulerMap[selectedGameObject] = euler;
                lastAnglesMap[selectedGameObject][0] = euler.x;
                lastAnglesMap[selectedGameObject][1] = euler.y;
                lastAnglesMap[selectedGameObject][2] = euler.z;
            }

            glm::vec3& rotationEuler = rotationEulerMap[selectedGameObject];
            float* lastAngles = lastAnglesMap[selectedGameObject];

            //update angles 
            if (editor->updatedAngles) {
                glm::quat q(transform->rotation.w, transform->rotation.x, transform->rotation.y, transform->rotation.z);
                glm::vec3 euler = glm::degrees(glm::eulerAngles(q));

                //normalized
                euler.x = normalizeAngle(euler.x);
                euler.y = normalizeAngle(euler.y);
                euler.z = normalizeAngle(euler.z);

                rotationEuler = euler;
                lastAngles[0] = euler.x;
                lastAngles[1] = euler.y;
                lastAngles[2] = euler.z;

                editor->updatedAngles = false;
            }

            //edit tab
            float angles[3] = { rotationEuler.x, rotationEuler.y, rotationEuler.z };
            ImGui::DragFloat3("Rotation", angles, 0.5f);

            //if the value was changed
            bool changed = false;
            for (int i = 0; i < 3; ++i)
            {
                if (angles[i] != lastAngles[i])
                {
                    changed = true;

                    //normalize
                    angles[i] = normalizeAngle(angles[i]);
                    lastAngles[i] = angles[i];
                }
            }

            //if it was changed we apply the rotation
            if (changed)
            {
                rotationEuler = glm::vec3(angles[0], angles[1], angles[2]);

                //convert to quat
                glm::quat newQuat = glm::quat(glm::radians(rotationEuler));
                transform->rotation.w = newQuat.w;
                transform->rotation.x = newQuat.x;
                transform->rotation.y = newQuat.y;
                transform->rotation.z = newQuat.z;
                editor->sceneModified = true;
            }

            //show quat (not editable)
            ImGui::Text("Rotation (Quaternion):");
            ImGui::Text("W: %.3f, X: %.3f, Y: %.3f, Z: %.3f",
                transform->rotation.w,
                transform->rotation.x,
                transform->rotation.y,
                transform->rotation.z);
        }
    }

    ImGui::Separator();
    ImGui::Text("Space Partitioning:");

    //change static or not
    bool wasStatic = selectedGameObject->isStatic;
    if (ImGui::Checkbox("Static", &selectedGameObject->isStatic))
    {
        if (wasStatic != selectedGameObject->isStatic)
        {
            //if we have changed static or dynamic we rebuild the quadtree
            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (opengl && opengl->useQuadtree)
            {
                opengl->RebuildQuadtree();
                LOG("GameObject " + selectedGameObject->name + " marked as " + (selectedGameObject->isStatic ? "STATIC" : "DYNAMIC"));
            }
            editor->sceneModified = true;
        }
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Static objects won't move during gameplay and\nare stored in the Quadtree for faster queries");
    }

    // Mesh Component
    if (ImGui::CollapsingHeader("Mesh"))
    {
        ComponentMesh* mesh = selectedGameObject->mesh;
        if (mesh && mesh->meshIndex >= 0 && mesh->meshIndex < (int)g_Meshes.size())
        {
            MeshData& meshData = g_Meshes[mesh->meshIndex];
            ImGui::Text("Mesh Index: %d", mesh->meshIndex);
            ImGui::Text("Path: %s", selectedGameObject->meshPath.c_str());
            ImGui::Text("Vertices: %d", meshData.numIndices);
            ImGui::Text("VAO: %u", meshData.VAO);
            ImGui::Text("VBO: %u", meshData.VBO);
            ImGui::Text("EBO: %u", meshData.EBO);

            ImGui::Separator();
            ImGui::Text("Normals Visualization");
            ImGui::Checkbox("Show Vertex Normals", &mesh->showVertexNormals);
            ImGui::Checkbox("Show Face Normals", &mesh->showFaceNormals);
        }
        else
        {
            ImGui::Text("No mesh assigned");
        }
    }

    //aabb component
    if (ImGui::CollapsingHeader("Bounding box"))
    {
        ComponentMesh* mesh = selectedGameObject->mesh;
        if (mesh && mesh->meshIndex >= 0 && mesh->meshIndex < (int)g_Meshes.size())
        {
            MeshData& meshData = g_Meshes[mesh->meshIndex];

            ImGui::Separator();

            //get world aabb for showing info
            WorldAABB worldAABB = mesh->GetWorldAABB();
            ImGui::Text("Current AABB Data");

            ImGui::Text("Min: (%.2f, %.2f, %.2f)", worldAABB.min.x, worldAABB.min.y, worldAABB.min.z);
            ImGui::Text("Max: (%.2f, %.2f, %.2f)", worldAABB.max.x, worldAABB.max.y, worldAABB.max.z);
            ImGui::Text("Center: (%.2f, %.2f, %.2f)", worldAABB.center.x, worldAABB.center.y, worldAABB.center.z);
            ImGui::Text("Size: (%.2f, %.2f, %.2f)", worldAABB.size.x, worldAABB.size.y, worldAABB.size.z);

            ImGui::Separator();
            ImGui::Text("Collision Visualization");

            //trigger aabb visualization
            if (ImGui::Checkbox("Show AABB", &mesh->showAABB))
            {
                if (mesh->showAABB) LOG("Enabled AABB visualization for " + selectedGameObject->name);
                else LOG("Disabled AABB visualization for " + selectedGameObject->name);
            }
        }
        else
        {
            ImGui::Text("No bounding box assigned");
        }
    }

    ImGui::Separator();

    // Texture Component
    if (ImGui::CollapsingHeader("Texture"))
    {
        ComponentTexture* texture = selectedGameObject->texture;
        if (texture && texture->hasTexture && texture->texturedata)
        {
            ImGui::Text("Texture ID: %u", texture->texturedata->id);
            ImGui::Text("Path: %s", texture->texturePath.c_str());

            // Show texture preview
            ImGui::Text("Texture Preview:");
            ImGui::Image((ImTextureID)(intptr_t)texture->texturedata->id, ImVec2(128, 128));

        }
        else
        {
            ImGui::Text("No texture assigned");
        }

        if (ImGui::Button("Load Texture...", ImVec2(-1, 0)))
        {
            std::string filepath = editor->OpenFileDialog(
                "Image Files (*.png;*.jpg;*.jpeg;*.tga;*.dds)\0*.png;*.jpg;*.jpeg;*.tga;*.dds\0All Files (*.*)\0*.*\0"
            );

            if (!filepath.empty())
            {
                if (texture->LoadTexture(filepath))
                {
                    LOG("Loaded texture: " + filepath);
                    editor->sceneModified = true;
                }
                else
                {
                    LOG_ERROR("Failed to load texture: " + filepath);
                }
            }
        }

        if (ImGui::Button("Use Checkerboard"))
        {
            // Delete old texture
            if (texture->hasTexture && texture->texturedata)
            {
                if (texture->texturedata->id != 0)
                    glDeleteTextures(1, &texture->texturedata->id);
            }
           
            editor->AssignCheckerboardTexture(selectedGameObject);
            LOG("Applied checkerboard texture to " + selectedGameObject->name);
            selectedGameObject->texture->texturePath = "";
        }

        //Drag and Drop Area for textures (also aviable directly to object)
        ImGui::Separator();
        ImGui::Text("Drag new texture in \ninspector tab or \n in object to change it!");
    }
}

void InspectorWindow::DrawMultiObjectInspector()
{
    ImGui::Text("Multiple objects selected (%d)", (int)editor->selectedGameObjects.size());
    ImGui::Separator();

    ImGui::Text("Selected GameObjects:");
    for (GameObject* go : editor->selectedGameObjects)
    {
        ImGui::BulletText("%s", go->name.c_str());
    }

    ImGui::Separator();

    //Transform   
    if (ImGui::CollapsingHeader("Multi-Object Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f),
            "Editing %d objects simultaneously", (int)editor->selectedGameObjects.size());
        ImGui::Spacing();

        // Calculate average values
        glm::vec3 avgPosition(0.0f);
        glm::vec3 avgScale(0.0f);
        glm::vec3 avgRotation(0.0f);

        int validCount = 0;
        for (GameObject* go : editor->selectedGameObjects)
        {
            if (go && go->transform)
            {
                avgPosition.x += go->transform->translation.x;
                avgPosition.y += go->transform->translation.y;
                avgPosition.z += go->transform->translation.z;

                avgScale.x += go->transform->scaling.x;
                avgScale.y += go->transform->scaling.y;
                avgScale.z += go->transform->scaling.z;

                // Convert quaternion to euler for averaging
                glm::quat q(go->transform->rotation.w, go->transform->rotation.x,
                    go->transform->rotation.y, go->transform->rotation.z);
                glm::vec3 euler = glm::degrees(glm::eulerAngles(q));
                avgRotation += euler;

                validCount++;
            }
        }

        if (validCount > 0)
        {
            avgPosition /= (float)validCount;
            avgScale /= (float)validCount;
            avgRotation /= (float)validCount;
        }

        // position
        float pos[3] = { avgPosition.x, avgPosition.y, avgPosition.z };
        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            // Calculate delta
            glm::vec3 delta(
                pos[0] - avgPosition.x,
                pos[1] - avgPosition.y,
                pos[2] - avgPosition.z
            );

            // Apply delta to all objects
            for (GameObject* go : editor->selectedGameObjects)
            {
                if (go && go->transform)
                {
                    go->transform->translation.x += delta.x;
                    go->transform->translation.y += delta.y;
                    go->transform->translation.z += delta.z;
                }
            }
            editor->sceneModified = true;
            editor->editing = true;
        }
        if (ImGui::IsItemDeactivated())
        {
            editor->editing = false;
        }

        // Rotation
        float rot[3] = { avgRotation.x, avgRotation.y, avgRotation.z };
        if (ImGui::DragFloat3("Rotation", rot, 0.5f))
        {
            // Calculate delta
            glm::vec3 delta(
                rot[0] - avgRotation.x,
                rot[1] - avgRotation.y,
                rot[2] - avgRotation.z
            );

            // Apply delta to all objects
            for (GameObject* go : editor->selectedGameObjects)
            {
                if (go && go->transform)
                {
                    // Get current rotation as euler
                    glm::quat currentQuat(go->transform->rotation.w, go->transform->rotation.x,
                        go->transform->rotation.y, go->transform->rotation.z);
                    glm::vec3 currentEuler = glm::degrees(glm::eulerAngles(currentQuat));

                    // Add delta
                    glm::vec3 newEuler = currentEuler + delta;

                    // Convert back to quaternion
                    glm::quat newQuat = glm::quat(glm::radians(newEuler));
                    go->transform->rotation.w = newQuat.w;
                    go->transform->rotation.x = newQuat.x;
                    go->transform->rotation.y = newQuat.y;
                    go->transform->rotation.z = newQuat.z;
                }
            }
            editor->sceneModified = true;
            editor->editing = true;
        }
        if (ImGui::IsItemDeactivated())
        {
            editor->editing = false;
        }

        // scale
        float scale[3] = { avgScale.x, avgScale.y, avgScale.z };
        if (ImGui::DragFloat3("Scale", scale, 0.01f, 0.01f, 100.0f))
        {
            // Calculate scale factor
            glm::vec3 scaleFactor(
                scale[0] / avgScale.x,
                scale[1] / avgScale.y,
                scale[2] / avgScale.z
            );

            // Apply scale factor to all objects
            for (GameObject* go : editor->selectedGameObjects)
            {
                if (go && go->transform)
                {
                    go->transform->scaling.x *= scaleFactor.x;
                    go->transform->scaling.y *= scaleFactor.y;
                    go->transform->scaling.z *= scaleFactor.z;
                }
            }
            editor->sceneModified = true;
            editor->editing = true;
        }
        if (ImGui::IsItemDeactivated())
        {
            editor->editing = false;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // reset
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Quick Actions:");

        if (ImGui::Button("Reset Position", ImVec2(-1, 0)))
        {
            for (GameObject* go : editor->selectedGameObjects)
            {
                if (go && go->transform)
                {
                    go->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
                }
            }
            editor->sceneModified = true;
            LOG("Reset position for " + std::to_string(editor->selectedGameObjects.size()) + " objects");
        }

        if (ImGui::Button("Reset Rotation", ImVec2(-1, 0)))
        {
            for (GameObject* go : editor->selectedGameObjects)
            {
                if (go && go->transform)
                {
                    go->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                }
            }
            editor->sceneModified = true;
            LOG("Reset rotation for " + std::to_string(editor->selectedGameObjects.size()) + " objects");
        }

        if (ImGui::Button("Reset Scale", ImVec2(-1, 0)))
        {
            for (GameObject* go : editor->selectedGameObjects)
            {
                if (go && go->transform)
                {
                    go->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);
                }
            }
            editor->sceneModified = true;
            LOG("Reset scale for " + std::to_string(editor->selectedGameObjects.size()) + " objects");
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}