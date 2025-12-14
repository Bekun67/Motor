#include "SceneViewportWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"
#include "EditorPlaySystem.h"
#include "Input.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "imgui_impl_sdl3.h"
#include <imgui.h>     
#include <ImGuizmo.h>  
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

SceneViewportWindow::SceneViewportWindow(ModuleEditor* editor)
    : EditorWindow(editor, "Scene")
{
}

void SceneViewportWindow::Draw()
{
    if (!visible) return;

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (editor->firstTimeSetup)
    {
        float x = windowWidth * editor->layout.sceneXPercent;
        float y = editor->layout.menuBarHeight + editor->layout.marginY;
        float width = windowWidth * editor->layout.sceneWidthPercent;
        float height = windowHeight * editor->layout.sceneHeightPercent - editor->layout.menuBarHeight;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (editor->useAdaptiveLayout)
    {
        float x = windowWidth * editor->layout.sceneXPercent;
        float y = editor->layout.menuBarHeight + editor->layout.marginY;
        float width = windowWidth * editor->layout.sceneWidthPercent;
        float height = windowHeight * editor->layout.sceneHeightPercent - editor->layout.menuBarHeight;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    std::string windowTitle = "Scene - " + editor->currentScenePath;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    // Add NoInputs flag to prevent window from capturing any input
    ImGuiWindowFlags sceneFlags = ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoInputs;

    ImGui::Begin(windowTitle.c_str(), nullptr, sceneFlags);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    editor->sceneViewportPos = viewportPos;
    editor->sceneViewportSize = viewportSize;

    ImGui::End();
    ImGui::PopStyleVar();

    // Draw play controls in a separate window that CAN receive input
    DrawPlayControls();

    // Handle drag and drop in a separate, invisible, input-enabled window
    HandleDragDropArea();
}

void SceneViewportWindow::HandleMeshDrop(const std::string& meshPath, float mouseX, float mouseY)
{
    LOG("Mesh dropped from Assets: " + meshPath);

    OpenGL* opengl = Application::GetInstance().opengl.get();
    Camera* camera = &opengl->camera;

    if (!opengl || !camera) return;

    int viewportWidth = (int)editor->sceneViewportSize.x;
    int viewportHeight = (int)editor->sceneViewportSize.y;

    float x = (2.0f * mouseX) / viewportWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / viewportHeight;

    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(camera->GetProjectionMatrix()) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::vec3(glm::inverse(camera->GetViewMatrix()) * rayEye);
    rayWorld = glm::normalize(rayWorld);

    glm::vec3 camPos = camera->GetPosition();
    float t = -camPos.y / rayWorld.y;
    glm::vec3 dropPosition = camPos + rayWorld * t;

    float normalizeScale = 1.0f;

    size_t meshCountBefore = g_Meshes.size();
    size_t instanceCountBefore = g_MeshInstances.size();

    if (LoadFile(meshPath.c_str()))
    {
        LOG("Model loaded successfully from Assets");

        //calculate global minimum Y from ALL geometry
        float globalMinY = FLT_MAX;

        for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
        {
            const MeshWithTransform& inst = g_MeshInstances[i];
            int meshIdx = inst.meshIndex;

            if (meshIdx >= 0 && meshIdx < (int)g_Meshes.size()) {
                const MeshData& meshData = g_Meshes[meshIdx];

                glm::vec4 worldMin = inst.transform * glm::vec4(meshData.aabbMin, 1.0f);
                globalMinY = std::min(globalMinY, worldMin.y);
            }
        }

        //count unique meshes
        Assimp::Importer counter;
        const aiScene* countScene = counter.ReadFile(meshPath.c_str(), aiProcess_Triangulate);
        int numMeshesInFBX = countScene ? countScene->mNumMeshes : 2;

        //game object for each model
        std::vector<GameObject*> createdObjects;

        for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
        {
            //create gameobject with mesh
            const MeshWithTransform& inst = g_MeshInstances[i];
            int meshIdx = inst.meshIndex;

            if (meshIdx < 0 || meshIdx >= (int)g_Meshes.size()) {
                continue;
            }

            MeshData& meshData = g_Meshes[meshIdx];

            glm::vec3 meshLocalCenter = (meshData.aabbMin + meshData.aabbMax) * 0.5f;

            glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
            GLint bufferSize;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

            int vertexSize = 8; // 3 pos + 3 normal + 2 uv
            int numVertices = bufferSize / (vertexSize * sizeof(float));

            std::vector<float> vertexData(bufferSize / sizeof(float));
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertexData.data());

            for (int v = 0; v < numVertices; ++v) {
                int offset = v * vertexSize;
                vertexData[offset + 0] -= meshLocalCenter.x;
                vertexData[offset + 1] -= meshLocalCenter.y;
                vertexData[offset + 2] -= meshLocalCenter.z;
            }

            glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertexData.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            meshData.aabbMin -= meshLocalCenter;
            meshData.aabbMax -= meshLocalCenter;
            meshData.center = glm::vec3(0, 0, 0);

            //create game object
            GameObject* go = new GameObject();
            int index = editor->CountNames("DroppedMesh_");
            go->name = "DroppedMesh_" + std::to_string(index);
            go->meshPath = meshPath;

            go->meshIndexInFBX = (i - instanceCountBefore) % numMeshesInFBX;

            go->mesh->meshIndex = meshIdx;

            glm::vec3 instancePosition, instanceScale;
            glm::quat instanceRotation;
            DecomposeTransform(inst.transform, instancePosition, instanceRotation, instanceScale);

            glm::vec4 meshWorldCenter4 = inst.transform * glm::vec4(meshLocalCenter, 1.0f);
            glm::vec3 meshWorldCenter = glm::vec3(meshWorldCenter4);

            glm::vec3 finalPos;
            finalPos.x = dropPosition.x + (meshWorldCenter.x - g_ModelCenter.x) * normalizeScale;
            finalPos.z = dropPosition.z + (meshWorldCenter.z - g_ModelCenter.z) * normalizeScale;
            finalPos.y = (meshWorldCenter.y - globalMinY) * normalizeScale;

            //change the translation to match the obtained coordinates
            go->transform->translation = aiVector3D(finalPos.x, finalPos.y, finalPos.z);

            //change the rotation to match the obtained rotation
            go->transform->rotation = aiQuaternion(
                instanceRotation.w,
                instanceRotation.x,
                instanceRotation.y,
                instanceRotation.z
            );

            //change the scale to match the obtained normalized scale
            go->transform->scaling = aiVector3D(
                instanceScale.x * normalizeScale,
                instanceScale.y * normalizeScale,
                instanceScale.z * normalizeScale
            );

            editor->AssignCheckerboardTexture(go);

            opengl->gameObjects.push_back(go);
            createdObjects.push_back(go);
        }

        if (!createdObjects.empty())
        {
            glm::vec3 groupCenter(0.0f);
            for (GameObject* obj : createdObjects)
            {
                groupCenter.x += obj->transform->translation.x;
                groupCenter.y += obj->transform->translation.y;
                groupCenter.z += obj->transform->translation.z;
            }
            groupCenter /= (float)createdObjects.size();

            GameObject* parentEmpty = new GameObject();
            std::string fileName = std::filesystem::path(meshPath).stem().string();
            int parentIndex = editor->CountNames(fileName + "_");
            parentEmpty->name = fileName + "_" + std::to_string(parentIndex);
            parentEmpty->meshPath = "";
            parentEmpty->meshIndexInFBX = -1;
            parentEmpty->mesh->meshIndex = -1;

            parentEmpty->transform->translation = aiVector3D(groupCenter.x, groupCenter.y, groupCenter.z);
            parentEmpty->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            parentEmpty->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            opengl->gameObjects.push_back(parentEmpty);

            int meshIndex = 0;
            for (GameObject* child : createdObjects)
            {
                child->name = parentEmpty->name + "_Mesh_" + std::to_string(meshIndex);
                meshIndex++;

                child->parent = parentEmpty;
                parentEmpty->children.push_back(child);
            }

            //logs
            LOG("=== FBX Import (Assets Window) ===");
            LOG("File: " + fileName);
            LOG("Created parent: " + parentEmpty->name + " at position (" +
                std::to_string(groupCenter.x) + ", " +
                std::to_string(groupCenter.y) + ", " +
                std::to_string(groupCenter.z) + ")");
            LOG("Total meshes imported: " + std::to_string(createdObjects.size()));
        }

        editor->sceneModified = true;
    }
    else
    {
        LOG_ERROR("Failed to load model from: " + meshPath);
    }
}

void SceneViewportWindow::HandleTextureDrop(const std::string& texturePath)
{
    LOG("Texture dropped from Assets: " + texturePath);

    if (!editor->selectedGameObjects.empty())
    {
        GameObject* selectedGO = editor->selectedGameObjects[0];

        if (selectedGO && selectedGO->texture)
        {
            if (selectedGO->texture->LoadTexture(texturePath))
            {
                LOG("Texture applied to: " + selectedGO->name);
                editor->sceneModified = true;
            }
            else
            {
                LOG_ERROR("Failed to load texture: " + texturePath);
            }
        }
    }
    else
    {
        LOG_WARNING("No GameObject selected to apply texture");
    }
}

void SceneViewportWindow::DrawPlayControls()
{
    bool isPlaying = EditorPlaySystem::IsPlaying();
    bool isPaused = EditorPlaySystem::IsPaused();
    bool isStopped = EditorPlaySystem::IsStopped();

    // Position on top of the Scene window
    float controlsX = editor->sceneViewportPos.x;
    float controlsY = editor->sceneViewportPos.y - 30.0f;

    // Button size
    ImVec2 buttonSize(80, 28);
    float totalWidth = buttonSize.x * 3 + ImGui::GetStyle().ItemSpacing.x * 2;

    controlsX += (editor->sceneViewportSize.x - totalWidth) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(controlsX, controlsY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(totalWidth + 20, buttonSize.y + 4));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 2));

    // This window can receive input 
    ImGuiWindowFlags playControlFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBackground;

    ImGui::Begin("##PlayControls", nullptr, playControlFlags);

    // PLAY button
    if (isPlaying && !isPaused)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));

    if (ImGui::Button("Play", buttonSize))
    {
        if (isStopped || isPaused)
        {
            EditorPlaySystem::Play();
        }
    }
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Start game simulation (F5)");

    ImGui::SameLine();

    // PAUSE button
    ImGui::BeginDisabled(isStopped);

    if (isPaused)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.4f, 0.2f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.5f, 0.1f, 1.0f));

    if (ImGui::Button("Pause", buttonSize))
    {
        if (isPlaying)
        {
            EditorPlaySystem::Pause();
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    if (ImGui::IsItemHovered() && !isStopped)
        ImGui::SetTooltip("Pause game simulation (F6)");

    ImGui::SameLine();

    // STOP button
    ImGui::BeginDisabled(isStopped);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Stop", buttonSize))
    {
        EditorPlaySystem::Stop();
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    if (ImGui::IsItemHovered() && !isStopped)
        ImGui::SetTooltip("Stop game and restore scene (F7)");

    ImGui::End();
    ImGui::PopStyleVar();
}

void SceneViewportWindow::HandleDragDropArea()
{
    bool isDragging = ImGui::GetDragDropPayload() != nullptr;

    if (!isDragging)
    {
        // No drag operation active, don't create the blocking window
        return;
    }

    // There's an active drag, create the drop target window
    ImGui::SetNextWindowPos(editor->sceneViewportPos);
    ImGui::SetNextWindowSize(editor->sceneViewportSize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

    ImGuiWindowFlags dropFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |

    ImGui::Begin("##SceneDropArea", nullptr, dropFlags);

    // Create invisible button to cover the viewport and accept drops
    ImGui::InvisibleButton("##dropzone", editor->sceneViewportSize);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MESH_FILE"))
        {
            const char* droppedPath = (const char*)payload->Data;
            if (droppedPath)
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                float relativeMouseX = mousePos.x - editor->sceneViewportPos.x;
                float relativeMouseY = mousePos.y - editor->sceneViewportPos.y;

                HandleMeshDrop(std::string(droppedPath), relativeMouseX, relativeMouseY);
            }
        }

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_FILE"))
        {
            const char* droppedPath = (const char*)payload->Data;
            if (droppedPath)
            {
                HandleTextureDrop(std::string(droppedPath));
            }
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}