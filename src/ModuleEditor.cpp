#include "ModuleEditor.h"
#include "Application.h"
#include "OpenGL.h"
#include "Window.h"
#include "Input.h"
#include "Camera.h"
#include "Texture.h"
#include "EditorPlaySystem.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "ComponentTransform.h"
#include "LoadFBX.h"
#include "PrimitiveGenerator.h"
#include "SceneSerializer.h"
#include "FileSystemManager.h"
#include "ResourceManager.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <IL/il.h>
#include <iostream>
#include <map>
#include <string>
#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo.h>
#include <windows.h>
#include <commdlg.h>

// window headers
#include "EditorWindow.h"
#include "SceneViewportWindow.h"
#include "HierarchyWindow.h"
#include "InspectorWindow.h"
#include "ConsoleWindow.h"
#include "ConfigurationWindow.h"
#include "AboutWindow.h"
#include "MenuBarWindow.h"
#include "AssetsWindow.h" 

ModuleEditor* g_Editor = nullptr;

void LOG(const std::string& message)
{
    if (g_Editor) g_Editor->AddLog(message, LogType::INFO);
    std::cout << "[INFO] " << message << std::endl;
}

void LOG_WARNING(const std::string& message)
{
    if (g_Editor) g_Editor->AddLog(message, LogType::WARNING);
    std::cout << "[WARNING] " << message << std::endl;
}

void LOG_ERROR(const std::string& message)
{
    if (g_Editor) g_Editor->AddLog(message, LogType::ERROR_LOG);
    std::cerr << "[ERROR] " << message << std::endl;
}

ModuleEditor::ModuleEditor()
{
    g_Editor = this;
}

ModuleEditor::~ModuleEditor()
{
    g_Editor = nullptr;
}

void ModuleEditor::UpdateLayout(int windowWidth, int windowHeight)
{
    if (windowWidth == lastWindowWidth && windowHeight == lastWindowHeight && !firstTimeSetup)
    {
        return;
    }

    lastWindowWidth = windowWidth;
    lastWindowHeight = windowHeight;

    if (firstTimeSetup)
    {
        return;
    }

    LOG("Updating layout for window size: " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight));
}

void ModuleEditor::ResetLayout()
{
    firstTimeSetup = true;
    LOG("Layout reset - will recalculate on next frame");
}

bool ModuleEditor::Start()
{
    LOG("ModuleEditor initialized successfully");

    LOG("=== System Information ===");
    LOG(std::string("SDL Version: ") + std::to_string(SDL_MAJOR_VERSION) + "." +
        std::to_string(SDL_MINOR_VERSION) + "." + std::to_string(SDL_MICRO_VERSION));

    const GLubyte* glVersion = glGetString(GL_VERSION);
    LOG(std::string("OpenGL Version: ") + (const char*)glVersion);

    const GLubyte* glRenderer = glGetString(GL_RENDERER);
    LOG(std::string("GPU: ") + (const char*)glRenderer);

    ILint devilVersion = ilGetInteger(IL_VERSION_NUM);
    LOG(std::string("DevIL Version: ") + std::to_string(devilVersion / 100) + "." +
        std::to_string((devilVersion % 100) / 10) + "." + std::to_string(devilVersion % 10));

    SetupImGuiStyle();
    LOG("Custom ImGui style applied");

    // Create all windows
    sceneViewportWindow = std::make_unique<SceneViewportWindow>(this);
    hierarchyWindow = std::make_unique<HierarchyWindow>(this);
    inspectorWindow = std::make_unique<InspectorWindow>(this);
    consoleWindow = std::make_unique<ConsoleWindow>(this);
    configurationWindow = std::make_unique<ConfigurationWindow>(this);
    aboutWindow = std::make_unique<AboutWindow>(this);
    menuBarWindow = std::make_unique<MenuBarWindow>(this);
    assetsWindow = std::make_unique<AssetsWindow>(this);

    LOG("All editor windows created");

    firstTimeSetup = true;

    return true;
}

bool ModuleEditor::PreUpdate()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return true;
}

bool ModuleEditor::Update()
{
    //std::cout << editing << std::endl;
    static Uint64 lastTime = SDL_GetTicks();
    Uint64 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    if (deltaTime > 0.0f)
    {
        float fps = 1.0f / deltaTime;
        fpsHistory.push_back(fps);
        if (fpsHistory.size() > maxFPSHistory)
            fpsHistory.erase(fpsHistory.begin());
    }

    ImGuizmo::BeginFrame();

    // Handle keyboard shortcuts
    bool ctrlPressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool shiftPressed = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

    if (!editing)
    {
        // Import Model (Ctrl+I)
        if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_I))
        {
            std::string filepath = OpenFileDialog(
                "3D Models (*.fbx;*.obj)\0*.fbx;*.obj\0All Files (*.*)\0*.*\0"
            );

            if (!filepath.empty())
            {
                OpenGL* opengl = Application::GetInstance().opengl.get();
                size_t meshCountBefore = g_Meshes.size();

                if (LoadFile(filepath.c_str()))
                {
                    for (size_t i = meshCountBefore; i < g_Meshes.size(); ++i)
                    {
                        GameObject* go = new GameObject();
                        int index = CountNames("ImportedMesh_");
                        go->name = "ImportedMesh_" + std::to_string(index);
                        go->meshPath = filepath;
                        go->meshIndexInFBX = (int)(i - meshCountBefore);

                        go->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
                        go->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                        go->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

                        go->mesh->meshIndex = (int)i;
                        AssignCheckerboardTexture(go);

                        opengl->gameObjects.push_back(go);

                        LOG("Imported model: " + filepath);
                    }

                    sceneModified = true;
                }
            }
        }

        // Unparent (Shift+P)
        if (shiftPressed && ImGui::IsKeyPressed(ImGuiKey_P))
        {
            if (!selectedGameObjects.empty())
            {
                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->parent)
                    {
                        GameObject* oldParent = go->parent;
                        auto command = std::make_unique<ReparentCommand>(go, oldParent, nullptr);
                        commandHistory.ExecuteCommand(std::move(command));
                        LOG("Unparented: " + go->name);
                    }
                }
                sceneModified = true;
            }
        }

        // Copy (Ctrl+C)
        if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_C) && !selectedGameObjects.empty())
        {
            CopySelectedObjects();
        }

        // Paste (Ctrl+V)
        if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_V) && HasCopiedObjects())
        {
            PasteObjects();
        }

        // Duplicate (Ctrl+D)
        if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_D) && !selectedGameObjects.empty())
        {
            DuplicateSelectedObjects();
        }

        // Undo (Ctrl+Z)
        if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_Z) && !shiftPressed)
        {
            if (commandHistory.CanUndo())
            {
                commandHistory.Undo();
            }
        }

        // Redo (Ctrl+Y or Ctrl+Shift+Z)
        if ((ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_Y)) ||
            (ctrlPressed && shiftPressed && ImGui::IsKeyPressed(ImGuiKey_Z)))
        {
            if (commandHistory.CanRedo())
            {
                commandHistory.Redo();
            }
        }

        // Save Scene (Ctrl+S)
        if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_S) && !shiftPressed)
        {
            if (currentScenePath.empty())
            {
                SaveSceneDialog();
            }
            else
            {
                SaveScene(currentScenePath);
            }
        }

        // Save Scene As (Ctrl+Shift+S)
        if (ctrlPressed && shiftPressed && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            SaveSceneDialog();
        }

		// Guizmo operation shortcuts
        if (ImGui::IsKeyPressed(ImGuiKey_W))
        {
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E))
        {
            currentGizmoOperation = ImGuizmo::ROTATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            currentGizmoOperation = ImGuizmo::SCALE;
        }
    }

    // Draw all windows
    menuBarWindow->Draw();
    sceneViewportWindow->Draw();

    if (showHierarchy)
        hierarchyWindow->Draw();

    if (showInspector)
        inspectorWindow->Draw();

    // Toggle between console and assets
    if (showConsole && !showAssets)
        consoleWindow->Draw();
    else if (showAssets && !showConsole)
        assetsWindow->Draw();

    if (showConfiguration)
        configurationWindow->Draw();

    if (showAbout)
        aboutWindow->Draw();

    DrawGuizmo();

    ProcessDeletions();

    return true;
}

bool ModuleEditor::PostUpdate()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_Window* window = Application::GetInstance().window->GetWindow();
    SDL_GL_SwapWindow(window);

    return true;
}

bool ModuleEditor::CleanUp()
{
    LOG("Cleaning up IMGUI");
    logs.clear();
    fpsHistory.clear();
    return true;
}

void ModuleEditor::AddLog(const std::string& message, LogType type)
{
    logs.emplace_back(message, type);
    if (logs.size() > maxLogs)
        logs.pop_front();
}

void ModuleEditor::ClearLog()
{
    logs.clear();
}

void ModuleEditor::DrawGuizmo()
{
    if (selectedGameObjects.empty())
        return;

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
        return;

    // See if there are static objects during PLAY
    if (EditorPlaySystem::IsPlaying())
    {
        bool hasStaticObjects = false;
        for (GameObject* go : selectedGameObjects)
        {
            if (go && go->isStatic)
            {
                hasStaticObjects = true;
                break;
            }
        }

        // if there ARE static objects, do not show the guizmo
        if (hasStaticObjects)
        {
            return;
        }
    }

    Camera* camera = &opengl->camera;
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    if (selectedGameObjects.size() == 1)
    {
        GameObject* selected = selectedGameObjects[0];
        if (!selected->transform)
            return;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(
            selected->transform->translation.x,
            selected->transform->translation.y,
            selected->transform->translation.z
        ));

        glm::quat quat(
            selected->transform->rotation.w,
            selected->transform->rotation.x,
            selected->transform->rotation.y,
            selected->transform->rotation.z
        );
        model *= glm::mat4_cast(quat);

        model = glm::scale(model, glm::vec3(
            selected->transform->scaling.x,
            selected->transform->scaling.y,
            selected->transform->scaling.z
        ));

        ImGuizmo::SetRect(
            sceneViewportPos.x,
            sceneViewportPos.y,
            sceneViewportSize.x,
            sceneViewportSize.y
        );

        glm::mat4 deltaMatrix = glm::mat4(1.0f);

        if (ImGuizmo::IsUsing() && !wasManipulating)
        {
            BeginTransformEdit(selected);
            wasManipulating = true;
        }

        if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            currentGizmoOperation,
            currentGizmoMode,
            glm::value_ptr(model),
            glm::value_ptr(deltaMatrix)))
        {
            if (!wasManipulating)
            {
                wasManipulating = true;

                glm::vec3 dummyTranslation, dummyScale;
                glm::quat dummyRotation;
                ComponentTransform tempTransform(nullptr);
                tempTransform.Decompose(model, dummyTranslation, dummyRotation, dummyScale);

                lastMultiSelectionRotation = dummyRotation;
                lastMultiSelectionScale = dummyScale;
            }

            editing = true;

            glm::vec3 newTranslation, newScale;
            glm::quat newRotation;

            selected->transform->Decompose(model, newTranslation, newRotation, newScale);

            selected->transform->translation.x = newTranslation.x;
            selected->transform->translation.y = newTranslation.y;
            selected->transform->translation.z = newTranslation.z;

            selected->transform->rotation.w = newRotation.w;
            selected->transform->rotation.x = newRotation.x;
            selected->transform->rotation.y = newRotation.y;
            selected->transform->rotation.z = newRotation.z;

            selected->transform->scaling.x = newScale.x;
            selected->transform->scaling.y = newScale.y;
            selected->transform->scaling.z = newScale.z;

            updatedAngles = true;
            sceneModified = true;
        }
        else
        {
            // Save state when finishing manipulation
            if (wasManipulating && !ImGuizmo::IsUsing())
            {
                EndTransformEdit(selected);
                wasManipulating = false;
                editing = false;
            }
        }
    }
    else
    {
        glm::vec3 selectionCenter = GetSelectionCenter();

        glm::mat4 model = glm::translate(glm::mat4(1.0f), selectionCenter);

        ImGuizmo::SetRect(
            sceneViewportPos.x,
            sceneViewportPos.y,
            sceneViewportSize.x,
            sceneViewportSize.y
        );

        glm::mat4 deltaMatrix = glm::mat4(1.0f);

        if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            currentGizmoOperation,
            currentGizmoMode,
            glm::value_ptr(model),
            glm::value_ptr(deltaMatrix)))
        {
            editing = true;

            glm::vec3 newCenter = glm::vec3(model[3]);

            if (currentGizmoOperation == ImGuizmo::TRANSLATE)
            {
                glm::vec3 offset = newCenter - selectionCenter;

                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        go->transform->translation.x += offset.x;
                        go->transform->translation.y += offset.y;
                        go->transform->translation.z += offset.z;
                    }
                }
            }
            else if (currentGizmoOperation == ImGuizmo::ROTATE)
            {
                glm::vec3 dummyTranslation, dummyScale;
                glm::quat newRotation;
                glm::mat4 rotationMatrix = model;
                rotationMatrix[3] = glm::vec4(0, 0, 0, 1);

                ComponentTransform tempTransform(nullptr);
                tempTransform.Decompose(model, dummyTranslation, newRotation, dummyScale);

                glm::quat deltaRotation = newRotation;
                lastMultiSelectionRotation = newRotation;

                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        glm::vec3 objectPos(
                            go->transform->translation.x,
                            go->transform->translation.y,
                            go->transform->translation.z
                        );

                        glm::vec3 offset = objectPos - selectionCenter;
                        glm::vec3 rotatedOffset = deltaRotation * offset;
                        glm::vec3 newPos = selectionCenter + rotatedOffset;

                        go->transform->translation.x = newPos.x;
                        go->transform->translation.y = newPos.y;
                        go->transform->translation.z = newPos.z;

                        glm::quat currentRot(
                            go->transform->rotation.w,
                            go->transform->rotation.x,
                            go->transform->rotation.y,
                            go->transform->rotation.z
                        );

                        glm::quat finalRot = deltaRotation * currentRot;

                        go->transform->rotation.w = finalRot.w;
                        go->transform->rotation.x = finalRot.x;
                        go->transform->rotation.y = finalRot.y;
                        go->transform->rotation.z = finalRot.z;
                    }
                }
            }
            else if (currentGizmoOperation == ImGuizmo::SCALE)
            {
                glm::vec3 dummyTranslation, currentScale;
                glm::quat dummyRotation;
                ComponentTransform tempTransform(nullptr);
                tempTransform.Decompose(model, dummyTranslation, dummyRotation, currentScale);

                glm::vec3 rawScaleFactor(1.0f);
                if (initialMultiSelectionScale.x > 0.0001f)
                    rawScaleFactor.x = currentScale.x / initialMultiSelectionScale.x;
                if (initialMultiSelectionScale.y > 0.0001f)
                    rawScaleFactor.y = currentScale.y / initialMultiSelectionScale.y;
                if (initialMultiSelectionScale.z > 0.0001f)
                    rawScaleFactor.z = currentScale.z / initialMultiSelectionScale.z;

                rawScaleFactor.x = glm::clamp(rawScaleFactor.x, 0.01f, 100.0f);
                rawScaleFactor.y = glm::clamp(rawScaleFactor.y, 0.01f, 100.0f);
                rawScaleFactor.z = glm::clamp(rawScaleFactor.z, 0.01f, 100.0f);

                glm::vec3 deltaScale = rawScaleFactor / lastMultiSelectionScale;

                deltaScale.x = glm::clamp(deltaScale.x, 0.5f, 2.0f);
                deltaScale.y = glm::clamp(deltaScale.y, 0.5f, 2.0f);
                deltaScale.z = glm::clamp(deltaScale.z, 0.5f, 2.0f);

                glm::vec3 smoothedScaleFactor = lastMultiSelectionScale * deltaScale;
                lastMultiSelectionScale = smoothedScaleFactor;

                static std::map<GameObject*, glm::vec3> originalScales;
                static std::map<GameObject*, glm::vec3> originalPositions;

                if (originalScales.empty())
                {
                    for (GameObject* go : selectedGameObjects)
                    {
                        if (go && go->transform)
                        {
                            originalScales[go] = glm::vec3(
                                go->transform->scaling.x,
                                go->transform->scaling.y,
                                go->transform->scaling.z
                            );

                            originalPositions[go] = glm::vec3(
                                go->transform->translation.x,
                                go->transform->translation.y,
                                go->transform->translation.z
                            );
                        }
                    }
                }

                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        glm::vec3 originalPos = originalPositions[go];
                        glm::vec3 offset = originalPos - selectionCenter;
                        glm::vec3 scaledOffset = offset * smoothedScaleFactor;
                        glm::vec3 newPos = selectionCenter + scaledOffset;

                        go->transform->translation.x = newPos.x;
                        go->transform->translation.y = newPos.y;
                        go->transform->translation.z = newPos.z;

                        glm::vec3 originalScale = originalScales[go];
                        go->transform->scaling.x = originalScale.x * smoothedScaleFactor.x;
                        go->transform->scaling.y = originalScale.y * smoothedScaleFactor.y;
                        go->transform->scaling.z = originalScale.z * smoothedScaleFactor.z;

                        go->transform->scaling.x = glm::clamp(go->transform->scaling.x, 0.001f, 1000.0f);
                        go->transform->scaling.y = glm::clamp(go->transform->scaling.y, 0.001f, 1000.0f);
                        go->transform->scaling.z = glm::clamp(go->transform->scaling.z, 0.001f, 1000.0f);
                    }
                }

                if (!ImGuizmo::IsUsing())
                {
                    originalScales.clear();
                    originalPositions.clear();
                }
            }

            sceneModified = true;
        }
        else
        {
            if (wasManipulating && !ImGuizmo::IsUsing())
            {
                lastMultiSelectionRotation = glm::quat(1, 0, 0, 0);
                lastMultiSelectionScale = glm::vec3(1, 1, 1);
                wasManipulating = false;
                editing = false;
            }
        }
    }
}

void ModuleEditor::AssignCheckerboardTexture(GameObject* go)
{
    const int size = 64;
    GLubyte checkerImage[64][64][4];
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            int c = ((((i & 0x8) == 0) ^ ((j & 0x8) == 0))) * 255;
            checkerImage[i][j][0] = (GLubyte)c;
            checkerImage[i][j][1] = (GLubyte)c;
            checkerImage[i][j][2] = (GLubyte)c;
            checkerImage[i][j][3] = (GLubyte)255;
        }
    }

    GLuint checkID;
    glGenTextures(1, &checkID);
    glBindTexture(GL_TEXTURE_2D, checkID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    go->texture->hasTexture = true;
    if (go->texture->texturedata == nullptr) {
        go->texture->texturedata = new TextureData();
    }
    go->texture->texturedata->id = checkID;
    go->texture->texturedata->type = "checkerboard";
    go->texture->texturedata->path = "checkerboard";
}

int ModuleEditor::CountNames(std::string prefix)
{
    OpenGL* opengl = Application::GetInstance().opengl.get();
    int maxIndex = -1;

    for (size_t i = 0; i < opengl->gameObjects.size(); i++)
    {
        GameObject* obj = opengl->gameObjects[i];

        bool matches = true;
        if (obj->name.size() <= prefix.size()) matches = false;
        else
        {
            for (size_t j = 0; j < prefix.size(); j++)
            {
                if (obj->name[j] != prefix[j])
                {
                    matches = false;
                    break;
                }
            }
        }

        if (matches)
        {
            std::string numberPart = obj->name.substr(prefix.size());
            int value = std::atoi(numberPart.c_str());

            if (value > maxIndex) maxIndex = value;
        }
    }

    return maxIndex + 1;
}

void ModuleEditor::RefreshScenesList()
{
    availableScenes.clear();

    std::string scenesDir = FileSystemManager::GetScenesDirectory();
    std::string ext = FileSystemManager::GetSceneExtension();

    if (!std::filesystem::exists(scenesDir))
    {
        std::filesystem::create_directories(scenesDir);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(scenesDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ext)
        {
            availableScenes.push_back(entry.path().filename().string());
        }
    }
}

void ModuleEditor::SaveSceneDialog()
{
    showSaveDialog = true;
}

void ModuleEditor::LoadSceneDialog()
{
    showLoadDialog = true;
    RefreshScenesList();
}

bool ModuleEditor::SaveScene(const std::string& filepath)
{
    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
    {
        LOG_ERROR("Failed to save scene: OpenGL module not available");
        return false;
    }

    if (SceneSerializer::SaveScene(filepath, opengl->gameObjects))
    {
        currentScenePath = filepath;
        sceneModified = false;
        LOG("Scene saved successfully: " + filepath);
        return true;
    }

    LOG_ERROR("Failed to save scene: " + filepath);
    return false;
}

bool ModuleEditor::LoadScene(const std::string& filepath)
{
    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
    {
        LOG_ERROR("Failed to load scene: OpenGL module not available");
        return false;
    }

    ClearCurrentScene();

    std::vector<GameObject*> loadedGameObjects;
    if (SceneSerializer::LoadScene(filepath, loadedGameObjects))
    {
        opengl->gameObjects = loadedGameObjects;

        currentScenePath = filepath;
        sceneModified = false;

        LOG("Scene loaded successfully: " + filepath);
        return true;
    }

    LOG_ERROR("Failed to load scene: " + filepath);
    return false;
}

void ModuleEditor::ClearCurrentScene()
{
    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
    {
        LOG_ERROR("Failed to clear scene: OpenGL module not available");
        return;
    }

    LOG("Clearing current scene...");

    selectedGameObjects.clear();
    opengl->selectedGameObject = nullptr;

    for (GameObject* go : opengl->gameObjects)
    {
        if (go != nullptr)
        {
            go->parent = nullptr;
            go->children.clear();
        }
    }

    for (GameObject* go : opengl->gameObjects)
    {
        if (go != nullptr)
        {
            go->m_IsBeingDestroyed = true;
            delete go;
        }
    }

    opengl->gameObjects.clear();

    LOG("Scene cleared successfully");
}

void ModuleEditor::SetupImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(15, 15);
    style.WindowRounding = 5.0f;
    style.FramePadding = ImVec2(5, 5);
    style.FrameRounding = 4.0f;
    style.ItemSpacing = ImVec2(12, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 15.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 5.0f;
    style.GrabRounding = 3.0f;

    style.Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 0.86f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
}

bool ModuleEditor::IsSelected(GameObject* go) const
{
    return std::find(selectedGameObjects.begin(), selectedGameObjects.end(), go) != selectedGameObjects.end();
}

void ModuleEditor::SelectGameObject(GameObject* go, bool includeDescendants)
{
    if (go == nullptr)
        return;

    selectedGameObjects.clear();

    selectedGameObjects.push_back(go);

    if (includeDescendants)
    {
        std::vector<GameObject*> descendants;
        go->GetAllDescendants(descendants);

        for (GameObject* descendant : descendants)
        {
            selectedGameObjects.push_back(descendant);
        }
    }

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (opengl && !selectedGameObjects.empty())
    {
        opengl->selectedGameObject = selectedGameObjects[0];
    }

    LOG("Selected " + std::to_string(selectedGameObjects.size()) + " GameObject(s)");
}

void ModuleEditor::DeselectAll()
{
    selectedGameObjects.clear();

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (opengl)
    {
        opengl->selectedGameObject = nullptr;
    }
}

glm::vec3 ModuleEditor::GetSelectionCenter() const
{
    if (selectedGameObjects.empty())
        return glm::vec3(0.0f);

    glm::vec3 center(0.0f);

    for (GameObject* go : selectedGameObjects)
    {
        if (go && go->transform)
        {
            center.x += go->transform->translation.x;
            center.y += go->transform->translation.y;
            center.z += go->transform->translation.z;
        }
    }

    float count = static_cast<float>(selectedGameObjects.size());
    return center / count;
}

void ModuleEditor::MarkForDeletion(GameObject* go)
{
    if (!go) return;

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl) return;

    // Create delete command for undo/redo
    auto command = std::make_unique<DeleteGameObjectCommand>(go);
    commandHistory.ExecuteCommand(std::move(command));

    LOG("Marked for deletion: " + go->name);

    std::vector<GameObject*> toMark;
    toMark.push_back(go);
    go->GetAllDescendants(toMark);

    for (GameObject* obj : toMark)
    {
        if (obj && !obj->m_MarkedForDeletion)
        {
            obj->m_MarkedForDeletion = true;

            if (std::find(m_ObjectsToDelete.begin(), m_ObjectsToDelete.end(), obj) == m_ObjectsToDelete.end())
            {
                m_ObjectsToDelete.push_back(obj);
            }
        }
    }

    DeselectAll();

    LOG("Marked " + std::to_string(toMark.size()) + " object(s) for deletion");
}

void ModuleEditor::ProcessDeletions()
{
    if (m_ObjectsToDelete.empty())
        return;

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
        return;

    std::vector<GameObject*> rootsToDelete;

    for (GameObject* go : m_ObjectsToDelete)
    {
        if (!go) continue;

        bool isRoot = (go->parent == nullptr);

        if (go->parent != nullptr)
        {
            if (std::find(m_ObjectsToDelete.begin(), m_ObjectsToDelete.end(), go->parent) == m_ObjectsToDelete.end())
            {
                isRoot = true;
            }
        }

        if (isRoot)
        {
            rootsToDelete.push_back(go);
        }
    }

    for (GameObject* go : m_ObjectsToDelete)
    {
        auto it = std::find(opengl->gameObjects.begin(), opengl->gameObjects.end(), go);
        if (it != opengl->gameObjects.end())
        {
            opengl->gameObjects.erase(it);
        }

        if (go->parent != nullptr)
        {
            go->parent->RemoveChild(go);
            go->parent = nullptr;
        }
    }

    for (GameObject* go : rootsToDelete)
    {
        if (go)
        {
            LOG("Deleting root GameObject: " + go->name);
            delete go;
        }
    }

    m_ObjectsToDelete.clear();
    sceneModified = true;

    LOG("Deletion processing complete");
}

void ModuleEditor::BeginTransformEdit(GameObject* go)
{
    if (!go || !go->transform) return;

    // If we're editing multiple objects, track them all
    if (selectedGameObjects.size() > 1)
    {
        if (!m_TrackingMultiTransform)
        {
            m_MultiTransformState.objects = selectedGameObjects;
            m_MultiTransformState.positions.clear();
            m_MultiTransformState.rotations.clear();
            m_MultiTransformState.scales.clear();

            for (GameObject* obj : selectedGameObjects)
            {
                if (!obj || !obj->transform) continue;

                m_MultiTransformState.positions.push_back(glm::vec3(
                    obj->transform->translation.x,
                    obj->transform->translation.y,
                    obj->transform->translation.z
                ));
                m_MultiTransformState.rotations.push_back(glm::quat(
                    obj->transform->rotation.w,
                    obj->transform->rotation.x,
                    obj->transform->rotation.y,
                    obj->transform->rotation.z
                ));
                m_MultiTransformState.scales.push_back(glm::vec3(
                    obj->transform->scaling.x,
                    obj->transform->scaling.y,
                    obj->transform->scaling.z
                ));
            }

            m_TrackingMultiTransform = true;
        }
    }
    else
    {
        // Single object tracking (original code)
        TransformState state;
        state.position = glm::vec3(
            go->transform->translation.x,
            go->transform->translation.y,
            go->transform->translation.z
        );
        state.rotation = glm::quat(
            go->transform->rotation.w,
            go->transform->rotation.x,
            go->transform->rotation.y,
            go->transform->rotation.z
        );
        state.scale = glm::vec3(
            go->transform->scaling.x,
            go->transform->scaling.y,
            go->transform->scaling.z
        );

        m_TransformStates[go] = state;
    }
}

void ModuleEditor::EndTransformEdit(GameObject* go)
{
    if (selectedGameObjects.size() > 1 && m_TrackingMultiTransform)
    {
        // Multi-object transform
        std::vector<glm::vec3> newPositions;
        std::vector<glm::quat> newRotations;
        std::vector<glm::vec3> newScales;

        bool hasChanged = false;

        for (size_t i = 0; i < selectedGameObjects.size(); ++i)
        {
            GameObject* obj = selectedGameObjects[i];
            if (!obj || !obj->transform) continue;

            glm::vec3 newPos(
                obj->transform->translation.x,
                obj->transform->translation.y,
                obj->transform->translation.z
            );
            glm::quat newRot(
                obj->transform->rotation.w,
                obj->transform->rotation.x,
                obj->transform->rotation.y,
                obj->transform->rotation.z
            );
            glm::vec3 newScale(
                obj->transform->scaling.x,
                obj->transform->scaling.y,
                obj->transform->scaling.z
            );

            newPositions.push_back(newPos);
            newRotations.push_back(newRot);
            newScales.push_back(newScale);

            if (i < m_MultiTransformState.positions.size())
            {
                if (m_MultiTransformState.positions[i] != newPos ||
                    m_MultiTransformState.rotations[i] != newRot ||
                    m_MultiTransformState.scales[i] != newScale)
                {
                    hasChanged = true;
                }
            }
        }

        if (hasChanged)
        {
            auto command = std::make_unique<MultiTransformCommand>(
                m_MultiTransformState.objects,
                m_MultiTransformState.positions,
                m_MultiTransformState.rotations,
                m_MultiTransformState.scales,
                newPositions,
                newRotations,
                newScales
            );

            commandHistory.ExecuteCommand(std::move(command));
        }

        m_TrackingMultiTransform = false;
        m_MultiTransformState.objects.clear();
        m_MultiTransformState.positions.clear();
        m_MultiTransformState.rotations.clear();
        m_MultiTransformState.scales.clear();
    }
    else if (go && go->transform)
    {
        // Single object transform 
        auto it = m_TransformStates.find(go);
        if (it == m_TransformStates.end()) return;

        TransformState& oldState = it->second;

        glm::vec3 newPos(
            go->transform->translation.x,
            go->transform->translation.y,
            go->transform->translation.z
        );
        glm::quat newRot(
            go->transform->rotation.w,
            go->transform->rotation.x,
            go->transform->rotation.y,
            go->transform->rotation.z
        );
        glm::vec3 newScale(
            go->transform->scaling.x,
            go->transform->scaling.y,
            go->transform->scaling.z
        );

        if (oldState.position != newPos ||
            oldState.rotation != newRot ||
            oldState.scale != newScale)
        {
            auto command = std::make_unique<TransformCommand>(
                go,
                oldState.position, oldState.rotation, oldState.scale,
                newPos, newRot, newScale
            );

            commandHistory.ExecuteCommand(std::move(command));
        }

        m_TransformStates.erase(it);
    }
}

void ModuleEditor::CopySelectedObjects()
{
    m_CopiedObjects.clear();

    for (GameObject* go : selectedGameObjects)
    {
        if (!go) continue;

        CopiedObjectData data;
        data.name = go->name;
        data.meshPath = go->meshPath;
        data.meshIndexInFBX = go->meshIndexInFBX;

        if (go->transform)
        {
            data.position = glm::vec3(
                go->transform->translation.x,
                go->transform->translation.y,
                go->transform->translation.z
            );
            data.rotation = glm::quat(
                go->transform->rotation.w,
                go->transform->rotation.x,
                go->transform->rotation.y,
                go->transform->rotation.z
            );
            data.scale = glm::vec3(
                go->transform->scaling.x,
                go->transform->scaling.y,
                go->transform->scaling.z
            );
        }

        if (go->texture)
        {
            data.texturePath = go->texture->texturePath;
        }

        data.originalParent = go->parent;

        m_CopiedObjects.push_back(data);
    }

    LOG("Copied " + std::to_string(m_CopiedObjects.size()) + " object(s)");
}

void ModuleEditor::PasteObjects()
{
    if (m_CopiedObjects.empty()) return;

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl) return;

    DeselectAll();

    for (const CopiedObjectData& data : m_CopiedObjects)
    {
        GameObject* newGO = new GameObject();

        // Create unique name
        int index = CountNames(data.name + "_");
        newGO->name = data.name + "_" + std::to_string(index);
        newGO->meshPath = data.meshPath;
        newGO->meshIndexInFBX = data.meshIndexInFBX;

        // Copy transform with offset
        if (newGO->transform)
        {
            newGO->transform->translation.x = data.position.x + 1.0f;
            newGO->transform->translation.y = data.position.y;
            newGO->transform->translation.z = data.position.z + 1.0f;

            newGO->transform->rotation.w = data.rotation.w;
            newGO->transform->rotation.x = data.rotation.x;
            newGO->transform->rotation.y = data.rotation.y;
            newGO->transform->rotation.z = data.rotation.z;

            newGO->transform->scaling.x = data.scale.x;
            newGO->transform->scaling.y = data.scale.y;
            newGO->transform->scaling.z = data.scale.z;
        }

        // Load mesh if it has one
        if (!data.meshPath.empty())
        {
            int engineMeshIndex = -1;
            if (ResourceManager::EnsureMeshExists(data.meshPath, data.meshIndexInFBX, engineMeshIndex))
            {
                newGO->mesh->meshIndex = engineMeshIndex;
            }
        }
        else
        {
            newGO->mesh->meshIndex = -1;
        }

        // Load texture
        if (!data.texturePath.empty() && data.texturePath != "checkerboard")
        {
            newGO->texture->LoadTexture(data.texturePath);
        }
        else
        {
            AssignCheckerboardTexture(newGO);
        }

        opengl->gameObjects.push_back(newGO);
        selectedGameObjects.push_back(newGO);

        LOG("Pasted: " + newGO->name);
    }

    sceneModified = true;
}

void ModuleEditor::DuplicateSelectedObjects()
{
    if (selectedGameObjects.empty()) return;

    CopySelectedObjects();
    PasteObjects();
}

std::string ModuleEditor::OpenFileDialog(const char* filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        return ofn.lpstrFile;
    }

    return "";
}

std::string ModuleEditor::SaveFileDialog(const char* filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn) == TRUE)
    {
        return ofn.lpstrFile;
    }

    return "";
}