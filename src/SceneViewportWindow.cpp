#include "SceneViewportWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"
#include "EditorPlaySystem.h"

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
    ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    editor->sceneViewportPos = viewportPos;
    editor->sceneViewportSize = viewportSize;

    ImGui::InvisibleButton("##SceneDropZone", viewportSize);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MESH_FILE"))
        {
            const char* droppedPath = (const char*)payload->Data;
            if (droppedPath)
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                float relativeMouseX = mousePos.x - viewportPos.x;
                float relativeMouseY = mousePos.y - viewportPos.y;

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
    ImGui::PopStyleVar();

    DrawPlayControls();
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

    size_t meshCountBefore = g_Meshes.size();
    size_t instanceCountBefore = g_MeshInstances.size();

    if (LoadFile(meshPath.c_str()))
    {
        LOG("Model loaded successfully from Assets");

        size_t newMeshCount = g_Meshes.size() - meshCountBefore;
        size_t newInstanceCount = g_MeshInstances.size() - instanceCountBefore;

        glm::vec3 globalMin(FLT_MAX);
        glm::vec3 globalMax(-FLT_MAX);

        for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
        {
            const MeshWithTransform& inst = g_MeshInstances[i];
            int meshIdx = inst.meshIndex;

            if (meshIdx >= 0 && meshIdx < (int)g_Meshes.size())
            {
                const MeshData& meshData = g_Meshes[meshIdx];

                glm::vec4 worldMin = inst.transform * glm::vec4(meshData.aabbMin, 1.0f);
                glm::vec4 worldMax = inst.transform * glm::vec4(meshData.aabbMax, 1.0f);

                globalMin = glm::min(globalMin, glm::vec3(worldMin));
                globalMin = glm::min(globalMin, glm::vec3(worldMax));
                globalMax = glm::max(globalMax, glm::vec3(worldMin));
                globalMax = glm::max(globalMax, glm::vec3(worldMax));
            }
        }

        glm::vec3 modelCenter = (globalMin + globalMax) * 0.5f;
        float modelSize = glm::length(globalMax - globalMin);

        float targetSize = 2.0f;
        float normalizeScale = (modelSize > 0.0001f) ? (targetSize / modelSize) : 1.0f;

        LOG("Model size: " + std::to_string(modelSize) + ", Scale factor: " + std::to_string(normalizeScale));

        GameObject* parentGO = new GameObject();
        int parentIndex = editor->CountNames("Model_");
        parentGO->name = "Model_" + std::to_string(parentIndex);
        parentGO->meshPath = meshPath;

        parentGO->transform->translation = aiVector3D(dropPosition.x, dropPosition.y, dropPosition.z);
        parentGO->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
        parentGO->transform->scaling = aiVector3D(normalizeScale, normalizeScale, normalizeScale);

        parentGO->mesh->meshIndex = -1;

        opengl->gameObjects.push_back(parentGO);
        LOG("Created parent GameObject: " + parentGO->name);

        for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
        {
            const MeshWithTransform& inst = g_MeshInstances[i];
            int meshIdx = inst.meshIndex;

            if (meshIdx < 0 || meshIdx >= (int)g_Meshes.size())
                continue;

            GameObject* childGO = new GameObject(parentGO);
            int childIndex = (int)(i - instanceCountBefore);
            childGO->name = "Mesh_" + std::to_string(childIndex);
            childGO->meshPath = meshPath;
            childGO->meshIndexInFBX = childIndex;

            glm::vec3 localPos, localScale;
            glm::quat localRot;
            DecomposeTransform(inst.transform, localPos, localRot, localScale);

            localPos -= modelCenter;

            childGO->transform->translation = aiVector3D(localPos.x, localPos.y, localPos.z);
            childGO->transform->rotation = aiQuaternion(localRot.w, localRot.x, localRot.y, localRot.z);
            childGO->transform->scaling = aiVector3D(localScale.x, localScale.y, localScale.z);

            childGO->mesh->meshIndex = meshIdx;
            editor->AssignCheckerboardTexture(childGO);

            opengl->gameObjects.push_back(childGO);
        }

        LOG("Created model with " + std::to_string(newInstanceCount) + " parts");

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

    // Position, on top of the Scene window
    float controlsX = editor->sceneViewportPos.x;
    float controlsY = editor->sceneViewportPos.y - 30.0f; 

    // buttons size
    ImVec2 buttonSize(80, 28);
    float totalWidth = buttonSize.x * 3 + ImGui::GetStyle().ItemSpacing.x * 2;

    controlsX += (editor->sceneViewportSize.x - totalWidth) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(controlsX, controlsY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(totalWidth + 20, buttonSize.y + 4));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 2));
    ImGui::Begin("##PlayControls", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBackground);

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