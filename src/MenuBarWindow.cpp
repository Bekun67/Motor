#include "MenuBarWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"
#include "OpenGL.h"
#include "GameObject.h"
#include "PrimitiveGenerator.h"
#include "FileSystemManager.h"
#include "EditorPlaySystem.h"
#include <assimp/Importer.hpp>   
#include <assimp/postprocess.h>  
#include <SDL3/SDL.h>
#include <filesystem>

MenuBarWindow::MenuBarWindow(ModuleEditor* editor)
    : EditorWindow(editor, "MenuBar")
{
}

void MenuBarWindow::Draw()
{
    if (!visible) return;

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (ImGui::BeginMainMenuBar())
    {
        float menuBarHeight = ImGui::GetWindowSize().y;
        editor->layout.menuBarHeight = menuBarHeight;

        DrawFileMenu();
        DrawEditMenu();
        DrawViewMenu();
        DrawGameObjectMenu();
        DrawHelpMenu();

        ImGui::EndMainMenuBar();
    }

    DrawPopups();
}

void MenuBarWindow::DrawFileMenu()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Scene"))
        {
            editor->showNewSceneConfirmation = true;
        }

        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
            if (editor->currentScenePath.empty())
            {
                editor->SaveSceneDialog();
            }
            else
            {
                editor->SaveScene(editor->currentScenePath);
            }
        }

        if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
        {
            editor->SaveSceneDialog();
        }

        if (ImGui::MenuItem("Load Scene"))
        {
            editor->LoadSceneDialog();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Import Model...", "Ctrl+I"))
        {
            std::string filepath = editor->OpenFileDialog(
                "3D Models (*.fbx;*.obj)\0*.fbx;*.obj\0All Files (*.*)\0*.*\0"
            );

            if (!filepath.empty())
            {
                OpenGL* opengl = Application::GetInstance().opengl.get();

                size_t meshCountBefore = g_Meshes.size();
                size_t instanceCountBefore = g_MeshInstances.size();

                if (LoadFile(filepath.c_str()))
                {
                    LOG("FBX loaded successfully");

                    // Calculate global bounding box for ALL meshes
                    glm::vec3 globalMin(FLT_MAX);
                    glm::vec3 globalMax(-FLT_MAX);

                    for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
                    {
                        const MeshWithTransform& inst = g_MeshInstances[i];
                        int meshIdx = inst.meshIndex;

                        if (meshIdx >= 0 && meshIdx < (int)g_Meshes.size())
                        {
                            const MeshData& meshData = g_Meshes[meshIdx];

                            // Transform AABB corners to world space
                            glm::vec3 corners[8] = {
                                glm::vec3(meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMin.z),
                                glm::vec3(meshData.aabbMax.x, meshData.aabbMin.y, meshData.aabbMin.z),
                                glm::vec3(meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMin.z),
                                glm::vec3(meshData.aabbMin.x, meshData.aabbMax.y, meshData.aabbMin.z),
                                glm::vec3(meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMax.z),
                                glm::vec3(meshData.aabbMax.x, meshData.aabbMin.y, meshData.aabbMax.z),
                                glm::vec3(meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMax.z),
                                glm::vec3(meshData.aabbMin.x, meshData.aabbMax.y, meshData.aabbMax.z)
                            };

                            for (int c = 0; c < 8; ++c)
                            {
                                glm::vec4 worldCorner = inst.transform * glm::vec4(corners[c], 1.0f);
                                glm::vec3 corner3 = glm::vec3(worldCorner);

                                globalMin = glm::min(globalMin, corner3);
                                globalMax = glm::max(globalMax, corner3);
                            }
                        }
                    }

                    glm::vec3 globalCenter = (globalMin + globalMax) * 0.5f;

                    // Calculate the minimum Y of the entire model
                    float globalMinY = FLT_MAX;
                    for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
                    {
                        const MeshWithTransform& inst = g_MeshInstances[i];
                        int meshIdx = inst.meshIndex;

                        if (meshIdx >= 0 && meshIdx < (int)g_Meshes.size())
                        {
                            const MeshData& meshData = g_Meshes[meshIdx];
                            glm::vec4 worldMin = inst.transform * glm::vec4(meshData.aabbMin, 1.0f);
                            globalMinY = std::min(globalMinY, worldMin.y);
                        }
                    }

                    // Count unique meshes in FBX
                    Assimp::Importer counter;
                    const aiScene* countScene = counter.ReadFile(filepath.c_str(), aiProcess_Triangulate);
                    int numMeshesInFBX = countScene ? countScene->mNumMeshes : 1;

                    // Create GameObjects
                    for (size_t i = instanceCountBefore; i < g_MeshInstances.size(); ++i)
                    {
                        const MeshWithTransform& inst = g_MeshInstances[i];
                        int meshIdx = inst.meshIndex;

                        if (meshIdx < 0 || meshIdx >= (int)g_Meshes.size())
                            continue;

                        MeshData& meshData = g_Meshes[meshIdx];

                        // Center vertices locally
                        glm::vec3 meshLocalCenter = (meshData.aabbMin + meshData.aabbMax) * 0.5f;

                        glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
                        GLint bufferSize;
                        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

                        int vertexSize = 8;
                        int numVertices = bufferSize / (vertexSize * sizeof(float));

                        std::vector<float> vertexData(bufferSize / sizeof(float));
                        glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertexData.data());

                        for (int v = 0; v < numVertices; ++v)
                        {
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

                        // Create GameObject
                        GameObject* go = new GameObject();
                        int index = editor->CountNames("ImportedMesh_");
                        go->name = "ImportedMesh_" + std::to_string(index);
                        go->meshPath = filepath;
                        go->meshIndexInFBX = (i - instanceCountBefore) % numMeshesInFBX;
                        go->mesh->meshIndex = meshIdx;

                        // Decompose transform
                        glm::vec3 instancePosition, instanceScale;
                        glm::quat instanceRotation;
                        DecomposeTransform(inst.transform, instancePosition, instanceRotation, instanceScale);

                        // Calculate world position of mesh center
                        glm::vec4 meshWorldCenter4 = inst.transform * glm::vec4(meshLocalCenter, 1.0f);
                        glm::vec3 meshWorldCenter = glm::vec3(meshWorldCenter4);

                        // Position relative to global center, on the floor (Y=0)
                        glm::vec3 finalPos;
                        finalPos.x = meshWorldCenter.x - globalCenter.x;
                        finalPos.y = meshWorldCenter.y - globalMinY;
                        finalPos.z = meshWorldCenter.z - globalCenter.z;

                        go->transform->translation = aiVector3D(finalPos.x, finalPos.y, finalPos.z);
                        go->transform->rotation = aiQuaternion(
                            instanceRotation.w,
                            instanceRotation.x,
                            instanceRotation.y,
                            instanceRotation.z
                        );
                        go->transform->scaling = aiVector3D(
                            instanceScale.x,
                            instanceScale.y,
                            instanceScale.z
                        );

                        // Load texture or use checkerboard
                        editor->AssignCheckerboardTexture(go);

                        opengl->gameObjects.push_back(go);
                        LOG("Created GameObject: " + go->name);
                    }

                    editor->sceneModified = true;
                    LOG("Imported " + std::to_string(g_MeshInstances.size() - instanceCountBefore) + " meshes from: " + filepath);
                }
                else
                {
                    LOG_ERROR("Failed to import model: " + filepath);
                }
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
            if (editor->sceneModified)
            {
                LOG_WARNING("Scene has unsaved changes!");
            }

            SDL_Event quitEvent;
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        }

        ImGui::EndMenu();
    }
}

void MenuBarWindow::DrawViewMenu()
{
    if (ImGui::BeginMenu("View"))
    {
        // Toggle between console and assets
        if (ImGui::MenuItem("Console", nullptr, editor->showConsole && !editor->showAssets))
        {
            editor->showConsole = true;
            editor->showAssets = false;
        }

        if (ImGui::MenuItem("Assets", nullptr, editor->showAssets && !editor->showConsole))
        {
            editor->showAssets = true;
            editor->showConsole = false;
        }

        ImGui::Separator();

        ImGui::MenuItem("Configuration", nullptr, &editor->showConfiguration);
        ImGui::MenuItem("Hierarchy", nullptr, &editor->showHierarchy);
        ImGui::MenuItem("Inspector", nullptr, &editor->showInspector);

        ImGui::EndMenu();
    }
}

void MenuBarWindow::DrawGameObjectMenu()
{
    if (ImGui::BeginMenu("GameObject"))
    {
        if (ImGui::MenuItem("Create Empty"))
        {
            LOG("Creating empty GameObject");

            GameObject* emptyGO = new GameObject();
            int index = editor->CountNames("GameObject_");
            emptyGO->name = "GameObject_" + std::to_string(index);

            emptyGO->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
            emptyGO->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            emptyGO->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            emptyGO->mesh->meshIndex = -1;
            emptyGO->meshPath = "";

            Application::GetInstance().opengl->gameObjects.push_back(emptyGO);
            editor->sceneModified = true;

            LOG("Created empty GameObject: " + emptyGO->name);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Create Cube"))
        {
            LOG("Creating cube primitive");

            float size = 2.0f;
            int meshIndex = PrimitiveGenerator::GenerateCube(size);

            GameObject* cube = new GameObject();
            int index = editor->CountNames("Cube_");
            cube->name = "Cube_" + std::to_string(index);
            cube->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::CUBE, size, 0, 0, 0);
            cube->meshIndexInFBX = 0;

            cube->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
            cube->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            cube->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            cube->mesh->meshIndex = meshIndex;
            editor->AssignCheckerboardTexture(cube);

            Application::GetInstance().opengl->gameObjects.push_back(cube);
            editor->sceneModified = true;

            LOG("Created GameObject: " + cube->name + " with meshIndex: " + std::to_string(meshIndex));
        }

        if (ImGui::MenuItem("Create Sphere"))
        {
            LOG("Creating sphere primitive");

            float radius = 1.0f;
            int segments = 32;
            int rings = 16;

            int meshIndex = PrimitiveGenerator::GenerateSphere(radius, segments, rings);

            GameObject* sphere = new GameObject();
            int index = editor->CountNames("Sphere_");
            sphere->name = "Sphere_" + std::to_string(index);
            sphere->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::SPHERE, radius, 0, segments, rings);
            sphere->meshIndexInFBX = 0;

            sphere->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
            sphere->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            sphere->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            sphere->mesh->meshIndex = meshIndex;
            editor->AssignCheckerboardTexture(sphere);

            Application::GetInstance().opengl->gameObjects.push_back(sphere);
            editor->sceneModified = true;

            LOG("Created GameObject: " + sphere->name + " with meshIndex: " + std::to_string(meshIndex));
        }

        if (ImGui::MenuItem("Create Cylinder"))
        {
            LOG("Creating cylinder primitive");

            float radius = 0.5f;
            float height = 2.0f;
            int segments = 32;

            int meshIndex = PrimitiveGenerator::GenerateCylinder(radius, height, segments);

            GameObject* cylinder = new GameObject();
            int index = editor->CountNames("Cylinder_");
            cylinder->name = "Cylinder_" + std::to_string(index);
            cylinder->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::CYLINDER, radius, height, segments, 0);
            cylinder->meshIndexInFBX = 0;

            cylinder->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
            cylinder->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            cylinder->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            cylinder->mesh->meshIndex = meshIndex;
            editor->AssignCheckerboardTexture(cylinder);

            Application::GetInstance().opengl->gameObjects.push_back(cylinder);
            editor->sceneModified = true;

            LOG("Created GameObject: " + cylinder->name + " with meshIndex: " + std::to_string(meshIndex));
        }

        if (ImGui::MenuItem("Create Plane"))
        {
            LOG("Creating plane primitive");

            float width = 5.0f;
            float depth = 5.0f;
            int widthSegments = 10;
            int depthSegments = 10;

            int meshIndex = PrimitiveGenerator::GeneratePlane(width, depth, widthSegments, depthSegments);

            GameObject* plane = new GameObject();
            int index = editor->CountNames("Plane_");
            plane->name = "Plane_" + std::to_string(index);

            plane->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
            plane->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            plane->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);
            plane->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::PLANE, width, depth, widthSegments, depthSegments);
            plane->meshIndexInFBX = 0;

            plane->mesh->meshIndex = meshIndex;
            editor->AssignCheckerboardTexture(plane);

            Application::GetInstance().opengl->gameObjects.push_back(plane);
            editor->sceneModified = true;

            LOG("Created GameObject: " + plane->name + " with meshIndex: " + std::to_string(meshIndex));
        }

        ImGui::EndMenu();
    }
}

void MenuBarWindow::DrawHelpMenu()
{
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("Documentation"))
        {
            std::string url = std::string(editor->repoURL) + "/blob/main/README.md";
            SDL_OpenURL(url.c_str());
            LOG("Opening documentation: " + url);
        }
        if (ImGui::MenuItem("Report a Bug"))
        {
            std::string url = std::string(editor->repoURL);
            SDL_OpenURL(url.c_str());
            LOG("Opening issues page: " + url);
        }
        if (ImGui::MenuItem("Download Latest"))
        {
            std::string url = std::string(editor->repoURL) + "/releases";
            SDL_OpenURL(url.c_str());
            LOG("Opening releases page: " + url);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("About"))
        {
            editor->showAbout = true;
        }

        ImGui::EndMenu();
    }
}

void MenuBarWindow::DrawPopups()
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    if (editor->showNewSceneConfirmation)
    {
        ImGui::OpenPopup("New Scene Confirmation");
        editor->showNewSceneConfirmation = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("New Scene Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        editor->editing = true;
        editor->sceneEditing = true;
        if (editor->sceneModified && !editor->currentScenePath.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: Current scene has unsaved changes!");
            ImGui::Spacing();
        }

        ImGui::Text("Are you sure you want to create a new scene?");
        ImGui::Text("All unsaved changes will be lost.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Confirm", ImVec2(120, 0)))
        {
            editor->ClearCurrentScene();
            editor->currentScenePath = "";
            editor->sceneModified = false;
            editor->editing = false;
            editor->sceneEditing = false;
            LOG("New scene created");
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            editor->editing = false;
            editor->sceneEditing = false;
            LOG("New scene creation cancelled");
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (editor->showSaveDialog)
    {
        ImGui::OpenPopup("Save Scene");
        editor->showSaveDialog = false;
    }

    if (ImGui::BeginPopupModal("Save Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter scene name:");
        ImGui::InputText("##scenename", editor->saveSceneNameBuffer, IM_ARRAYSIZE(editor->saveSceneNameBuffer));

        ImGui::Separator();
        editor->editing = true;
        editor->sceneEditing = true;

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            std::string sceneName = std::string(editor->saveSceneNameBuffer);
            if (!sceneName.empty())
            {
                std::string filepath = FileSystemManager::GetScenesDirectory() +
                    sceneName +
                    FileSystemManager::GetSceneExtension();
                editor->SaveScene(filepath);
                editor->editing = false;
                editor->sceneEditing = false;
                ImGui::CloseCurrentPopup();
            }
            else
            {
                LOG_WARNING("Scene name cannot be empty");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            editor->editing = false;
            editor->sceneEditing = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (editor->showLoadDialog)
    {
        ImGui::OpenPopup("Load Scene");
        editor->showLoadDialog = false;
    }

    if (ImGui::BeginPopupModal("Load Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        editor->editing = true;
        editor->sceneEditing = true;
        ImGui::Text("Available scenes:");
        ImGui::Separator();

        if (editor->availableScenes.empty())
        {
            ImGui::Text("No saved scenes found");
        }
        else
        {
            for (const auto& sceneName : editor->availableScenes)
            {
                if (ImGui::Selectable(sceneName.c_str()))
                {
                    editor->pendingSceneToLoad = FileSystemManager::GetScenesDirectory() + sceneName;
                    editor->showLoadSceneConfirmation = true;
                    editor->editing = false;
                    editor->sceneEditing = false;
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            editor->editing = false;
            editor->sceneEditing = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (editor->showLoadSceneConfirmation)
    {
        ImGui::OpenPopup("Load Scene Confirmation");
        editor->showLoadSceneConfirmation = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Load Scene Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (editor->sceneModified && !editor->currentScenePath.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: Current scene has unsaved changes!");
            ImGui::Spacing();
        }

        ImGui::Text("Are you sure you want to load a different scene?");
        ImGui::Text("All unsaved changes will be lost.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Confirm", ImVec2(120, 0)))
        {
            if (!editor->pendingSceneToLoad.empty())
            {
                editor->LoadScene(editor->pendingSceneToLoad);
                editor->pendingSceneToLoad = "";
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            LOG("Load scene cancelled");
            editor->pendingSceneToLoad = "";
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void MenuBarWindow::DrawEditMenu()
{
    if (ImGui::BeginMenu("Edit"))
    {
        bool canUndo = editor->commandHistory.CanUndo();
        bool canRedo = editor->commandHistory.CanRedo();

        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
        {
            editor->commandHistory.Undo();
        }

        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo))
        {
            editor->commandHistory.Redo();
        }

        ImGui::Separator();

        bool hasSelection = !editor->selectedGameObjects.empty();

        if (ImGui::MenuItem("Copy", "Ctrl+C", false, hasSelection))
        {
            editor->CopySelectedObjects();
        }

        if (ImGui::MenuItem("Paste", "Ctrl+V", false, editor->HasCopiedObjects()))
        {
            editor->PasteObjects();
        }

        if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSelection))
        {
            editor->DuplicateSelectedObjects();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete", "Delete", false, hasSelection))
        {
            for (GameObject* go : editor->selectedGameObjects)
            {
                editor->MarkForDeletion(go);
            }
        }

        ImGui::EndMenu();
    }
}