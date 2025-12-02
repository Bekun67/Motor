#include "ModuleEditor.h"
#include "Application.h"
#include "OpenGL.h"
#include "Window.h"
#include "Input.h"
#include "Camera.h"
#include "Texture.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "ComponentTransform.h"
#include "LoadFBX.h"
#include "PrimitiveGenerator.h"
#include "SceneSerializer.h"
#include "FileSystemManager.h"
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


ModuleEditor* g_Editor = nullptr;
static char nameBuffer[128] = "";

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
    // Only update if window size actually changed
    if (windowWidth == lastWindowWidth && windowHeight == lastWindowHeight && !firstTimeSetup)
    {
        return;
    }

    lastWindowWidth = windowWidth;
    lastWindowHeight = windowHeight;

    // Disable adaptive layout during first time setup
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

    // Log system information
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

	// Apply custom ImGui style from Dougbinks
    SetupImGuiStyle();
    LOG("Custom ImGui style applied");

    firstTimeSetup = true;

    return true;
}

bool ModuleEditor::PreUpdate()
{
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return true;
}

bool ModuleEditor::Update()
{
    // Calculate FPS
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

    // Handle Gizmo operation changes with W, E, R keys
    if (!editing)
    {
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

    // Draw all editor windows
    DrawSceneViewport();
    DrawHierarchy();
    DrawInspector();
    DrawConsole();
    DrawMenuBar();
    if (showConfiguration) DrawConfiguration();
    if (showAbout) DrawAbout();

    DrawGuizmo();

    // Process deletions at the END of the frame, after all ImGui operations
    ProcessDeletions();

    return true;
}

bool ModuleEditor::PostUpdate()
{
    // Render ImGui at the end of the frame
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
    // Delete the first log if we have too many
    if (logs.size() > maxLogs)
        logs.pop_front();
}

void ModuleEditor::ClearLog()
{
    logs.clear();
}

void ModuleEditor::DrawSceneViewport()
{
    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (firstTimeSetup)
    {
        float x = windowWidth * layout.sceneXPercent;
        float y = layout.menuBarHeight + layout.marginY;
        float width = windowWidth * layout.sceneWidthPercent;
        float height = windowHeight * layout.sceneHeightPercent - layout.menuBarHeight;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (useAdaptiveLayout)
    {
        float x = windowWidth * layout.sceneXPercent;
        float y = layout.menuBarHeight + layout.marginY;
        float width = windowWidth * layout.sceneWidthPercent;
        float height = windowHeight * layout.sceneHeightPercent - layout.menuBarHeight;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    std::string windowTitle = "Scene - " + currentScenePath;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    sceneViewportPos = viewportPos;
    sceneViewportSize = viewportSize;

    ImGui::End();
    ImGui::PopStyleVar();
}

void ModuleEditor::DrawMenuBar()
{
    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    // Menu bar horizontal estilo Unity
    if (ImGui::BeginMainMenuBar())
    {
        float menuBarHeight = ImGui::GetWindowSize().y;
        layout.menuBarHeight = menuBarHeight;

        // File Menu
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene"))
            {
                // Show confirmation dialog instead of immediately clearing
                showNewSceneConfirmation = true;
            }

            if (ImGui::MenuItem("Save Scene"))
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

            if (ImGui::MenuItem("Save Scene As..."))
            {
                SaveSceneDialog();
            }

            if (ImGui::MenuItem("Load Scene"))
            {
                LoadSceneDialog();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                if (sceneModified)
                {
                    LOG_WARNING("Scene has unsaved changes!");
                }

                SDL_Event quitEvent;
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }

            ImGui::EndMenu();
        }

        // View Menu
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Console", nullptr, &showConsole);
            ImGui::MenuItem("Configuration", nullptr, &showConfiguration);
            ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &showInspector);

            ImGui::EndMenu();
        }

        // GameObject Menu
        if (ImGui::BeginMenu("GameObject"))
        {
            if (ImGui::MenuItem("Create Empty"))
            {
                LOG("Creating empty GameObject");

                GameObject* emptyGO = new GameObject();
                int index = CountNames("GameObject_");
                emptyGO->name = "GameObject_" + std::to_string(index);

                emptyGO->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
                emptyGO->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                emptyGO->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

                emptyGO->mesh->meshIndex = -1;
                emptyGO->meshPath = "";

                Application::GetInstance().opengl->gameObjects.push_back(emptyGO);
                sceneModified = true;

                LOG("Created empty GameObject: " + emptyGO->name);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Create Cube"))
            {
                LOG("Creating cube primitive");

                float size = 2.0f;
                int meshIndex = PrimitiveGenerator::GenerateCube(size);

                GameObject* cube = new GameObject();
                int index = CountNames("Cube_");
                cube->name = "Cube_" + std::to_string(index);
                cube->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::CUBE, size, 0, 0, 0);
                cube->meshIndexInFBX = 0;

                cube->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
                cube->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                cube->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

                cube->mesh->meshIndex = meshIndex;
                AssignCheckerboardTexture(cube);

                Application::GetInstance().opengl->gameObjects.push_back(cube);
                sceneModified = true;

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
                int index = CountNames("Sphere_");
                sphere->name = "Sphere_" + std::to_string(index);
                sphere->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::SPHERE, radius, 0, segments, rings);
                sphere->meshIndexInFBX = 0;

                sphere->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
                sphere->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                sphere->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

                sphere->mesh->meshIndex = meshIndex;
                AssignCheckerboardTexture(sphere);

                Application::GetInstance().opengl->gameObjects.push_back(sphere);
                sceneModified = true;

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
                int index = CountNames("Cylinder_");
                cylinder->name = "Cylinder_" + std::to_string(index);
                cylinder->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::CYLINDER, radius, height, segments, 0);
                cylinder->meshIndexInFBX = 0;

                cylinder->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
                cylinder->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                cylinder->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

                cylinder->mesh->meshIndex = meshIndex;
                AssignCheckerboardTexture(cylinder);

                Application::GetInstance().opengl->gameObjects.push_back(cylinder);
                sceneModified = true;

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
                int index = CountNames("Plane_");
                plane->name = "Plane_" + std::to_string(index);

                plane->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
                plane->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                plane->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);
                plane->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::PLANE, width, depth, widthSegments, depthSegments);
                plane->meshIndexInFBX = 0;

                plane->mesh->meshIndex = meshIndex;
                AssignCheckerboardTexture(plane);

                Application::GetInstance().opengl->gameObjects.push_back(plane);
                sceneModified = true;

                LOG("Created GameObject: " + plane->name + " with meshIndex: " + std::to_string(meshIndex));
            }

            ImGui::EndMenu();
        }

        // Help Menu
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Documentation"))
            {
                std::string url = std::string(repoURL) + "/blob/main/README.md";
                SDL_OpenURL(url.c_str());
                LOG("Opening documentation: " + url);
            }
            if (ImGui::MenuItem("Report a Bug"))
            {
                std::string url = std::string(repoURL);
                SDL_OpenURL(url.c_str());
                LOG("Opening issues page: " + url);
            }
            if (ImGui::MenuItem("Download Latest"))
            {
                std::string url = std::string(repoURL) + "/releases";
                SDL_OpenURL(url.c_str());
                LOG("Opening releases page: " + url);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About"))
            {
                showAbout = true;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

	// newscene popup
    if (showNewSceneConfirmation)
    {
        ImGui::OpenPopup("New Scene Confirmation");
        showNewSceneConfirmation = false;
    }

    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("New Scene Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (sceneModified && !currentScenePath.empty())
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
            ClearCurrentScene();

            currentScenePath = "";
            sceneModified = false;
            LOG("New scene created");

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            LOG("New scene creation cancelled");
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // savescene popup
    if (showSaveDialog)
    {
        ImGui::OpenPopup("Save Scene");
        showSaveDialog = false;
    }

    if (ImGui::BeginPopupModal("Save Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter scene name:");
        ImGui::InputText("##scenename", saveSceneNameBuffer, IM_ARRAYSIZE(saveSceneNameBuffer));

        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            std::string sceneName = std::string(saveSceneNameBuffer);
            if (!sceneName.empty())
            {
                std::string filepath = FileSystemManager::GetScenesDirectory() +
                    sceneName +
                    FileSystemManager::GetSceneExtension();
                SaveScene(filepath);
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
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

	// load scene popup
    if (showLoadDialog)
    {
        ImGui::OpenPopup("Load Scene");
        showLoadDialog = false;
    }

    if (ImGui::BeginPopupModal("Load Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Available scenes:");
        ImGui::Separator();

        if (availableScenes.empty())
        {
            ImGui::Text("No saved scenes found");
        }
        else
        {
            for (const auto& sceneName : availableScenes)
            {
                if (ImGui::Selectable(sceneName.c_str()))
                {
                    // Store the scene to load and show confirmation
                    pendingSceneToLoad = FileSystemManager::GetScenesDirectory() + sceneName;
                    showLoadSceneConfirmation = true;
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // confirm loadscene
    if (showLoadSceneConfirmation)
    {
        ImGui::OpenPopup("Load Scene Confirmation");
        showLoadSceneConfirmation = false;
    }

    // Center the popup
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Load Scene Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (sceneModified && !currentScenePath.empty())
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
            if (!pendingSceneToLoad.empty())
            {
                LoadScene(pendingSceneToLoad);
                pendingSceneToLoad = "";
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            LOG("Load scene cancelled");
            pendingSceneToLoad = "";
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ModuleEditor::DrawConsole()
{

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (firstTimeSetup)
    {
        float x = windowWidth * layout.consoleXPercent + layout.marginX;
        float y = windowHeight * layout.consoleYPercent;
        float width = windowWidth * layout.consoleWidthPercent;
        float height = windowHeight * layout.consoleHeightPercent - layout.marginY;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (useAdaptiveLayout)
    {
        float x = windowWidth * layout.consoleXPercent;
        float y = windowHeight * layout.consoleYPercent;
        float width = windowWidth * layout.consoleWidthPercent;
        float height = windowHeight * layout.consoleHeightPercent - layout.marginY;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Console", &showConsole);

    if (ImGui::Button("Clear"))
        ClearLog();

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& log : logs)
    {
        ImVec4 color;
        switch (log.type)
        {
        case LogType::INFO:
            color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            break;
        case LogType::WARNING:
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            break;
        case LogType::ERROR_LOG:
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            break;
        }
        ImGui::TextColored(color, "%s", log.message.c_str());
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

void ModuleEditor::DrawConfiguration()
{
    if (firstTimeSetup)
    {
        ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
    }

    ImGui::Begin("Configuration", &showConfiguration);

    if (ImGui::CollapsingHeader("Editor Layout", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped("Reset all ImGui windows to their default positions and sizes.");
        ImGui::Spacing();

        if (ImGui::Button("Reset Layout", ImVec2(-1, 0)))
        {
            ResetLayout();
            LOG("Resetting editor layout to default positions");
        }

        ImGui::Spacing();
        ImGui::Checkbox("Adaptive Layout", &useAdaptiveLayout);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Automatically adjust layout when window is resized");
        }

        ImGui::Spacing();
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("Application", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // FPS Graph
        if (!fpsHistory.empty())
        {
            float avgFPS = 0.0f;
            for (float fps : fpsHistory)
                avgFPS += fps;
            avgFPS /= fpsHistory.size();

            char title[64];
            sprintf(title, "Framerate %.1f FPS", avgFPS);
            ImGui::PlotHistogram("##framerate", fpsHistory.data(), (int)fpsHistory.size(),
                0, title, 0.0f, 120.0f, ImVec2(0, 80));
        }

        ImGui::Text("Frame Time: %.3f ms", lastFrameTime * 1000.0f);
    }

//frustum culling
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Camera* cam = &Application::GetInstance().opengl->camera;
        ComponentCamera* editorCam = cam->GetCameraComponent();

        if (editorCam)
        {
            // FOV
            float fov = editorCam->GetFOV();
            if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f))
            {
                editorCam->SetFOV(fov);
                LOG("Camera FOV changed to " + std::to_string(fov));
            }

            // Near Plane
            float nearPlane = editorCam->GetNearPlane();
            if (ImGui::DragFloat("Near Plane", &nearPlane, 0.1f, 0.1f, 10.0f))
            {
                editorCam->SetNearPlane(nearPlane);
                LOG("Camera Near Plane changed to " + std::to_string(nearPlane));
            }

            // Far Plane
            float farPlane = editorCam->GetFarPlane();
            if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, 10.0f, 10000.0f))
            {
                editorCam->SetFarPlane(farPlane);
                LOG("Camera Far Plane changed to " + std::to_string(farPlane));
            }

            ImGui::Separator();

            // Frustum Culling Toggle
            if (ImGui::Checkbox("Enable Frustum Culling", &cam->frustumCullingEnabled))
            {
                if (cam->frustumCullingEnabled)
                {
                    LOG("Frustum Culling ENABLED");
                }
                else
                {
                    LOG("Frustum Culling DISABLED");
                }
            }

            // Statistics
            if (cam->frustumCullingEnabled)
            {
                OpenGL* opengl = Application::GetInstance().opengl.get();
                ImGui::Text("Objects Rendered: %d", opengl->renderedCount);
                ImGui::Text("Objects Culled: %d", opengl->culledCount);

                int total = opengl->renderedCount + opengl->culledCount;
                if (total > 0)
                {
                    float percentage = (float)opengl->culledCount / (float)total * 100.0f;
                    ImGui::Text("Culling Efficiency: %.1f%%", percentage);
                }
            }

            // Debug Raycast Toggle
            if (ImGui::Checkbox("Show Raycast to Game Objects", &editorCam->debugRaycastEnabled))
            {
                if (editorCam->debugRaycastEnabled)
                {
                    LOG("Raycast to Game Objects ENABLED");
                }
                else
                {
                    LOG("Raycast to Game Objects DISABLED");
                }
            }

            // Z-Buffer Debug Toggle
            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (ImGui::Checkbox("Show Z-Buffer Depth Debug", &opengl->debugZBuffer))
            {
                if (opengl->debugZBuffer)
                {
                    LOG("Z-Buffer ENABLED");
                }
                else
                {
                    LOG("Z-Buffer DISABLED");
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Window"))
    {
        Window* window = Application::GetInstance().window.get();
        if (window)
        {
            int width, height;
            window->GetWindowSize(width, height);
            ImGui::Text("Width: %d", width);
            ImGui::Text("Height: %d", height);
            ImGui::Text("Scale: %d", window->GetScale());
        }
    }

    if (ImGui::CollapsingHeader("Renderer"))
    {
        OpenGL* opengl = Application::GetInstance().opengl.get();
        if (opengl)
        {
            ImGui::Checkbox("Show Grid", &opengl->showGrid);

            //vertex normals
            if (ImGui::Checkbox("Show All Vertex Normals", &showAllVertexNormals))
            {
                //apply to all GameObjects in scene
                for (GameObject* go : opengl->gameObjects)
                {
                    if (go && go->mesh)
                    {
                        go->mesh->showVertexNormals = showAllVertexNormals;
                    }
                }

                if (showAllAABBs) LOG("Enabled Vertex normals visualization for all GameObjects");
                else LOG("Disabled Vertex Normals visualization for all GameObjects");
            }

            //face normals
            if (ImGui::Checkbox("Show All Face Normals", &showAllFaceNormals))
            {
                //apply to all GameObjects in scene
                for (GameObject* go : opengl->gameObjects)
                {
                    if (go && go->mesh)
                    {
                        go->mesh->showFaceNormals = showAllFaceNormals;
                    }
                }

                if (showAllAABBs) LOG("Enabled Face normals visualization for all GameObjects");
                else LOG("Disabled Face Normals visualization for all GameObjects");
            }

            //aabb viewer
            if (ImGui::Checkbox("Show All AABBs", &showAllAABBs))
            {
                //apply to all GameObjects in scene
                for (GameObject* go : opengl->gameObjects)
                {
                    if (go && go->mesh)
                    {
                        go->mesh->showAABB = showAllAABBs;
                    }
                }

                if (showAllAABBs) LOG("Enabled AABB visualization for all GameObjects");
                else LOG("Disabled AABB visualization for all GameObjects");
            }
            ImGui::Text("GameObjects in scene: %zu", opengl->gameObjects.size());
        }
    }

    if (ImGui::CollapsingHeader("Hardware"))
    {
        const GLubyte* glVersion = glGetString(GL_VERSION);
        ImGui::Text("OpenGL Version: %s", glVersion);

        const GLubyte* glRenderer = glGetString(GL_RENDERER);
        ImGui::Text("GPU: %s", glRenderer);

        const GLubyte* glVendor = glGetString(GL_VENDOR);
        ImGui::Text("GPU Vendor: %s", glVendor);

        ILint devilVersion = ilGetInteger(IL_VERSION_NUM);
        ImGui::Text("DevIL Version: %d.%d.%d",
            devilVersion / 100,
            (devilVersion % 100) / 10,
            devilVersion % 10);
    }

    if (ImGui::CollapsingHeader("Memory"))
    {
        ImGui::Text("Total meshes loaded: %zu", g_Meshes.size());

        size_t totalVertices = 0;
        for (const auto& mesh : g_Meshes)
            totalVertices += mesh.numIndices;
        ImGui::Text("Total indices: %zu", totalVertices);
    }

    ImGui::End();
}

void ModuleEditor::DrawHierarchy()
{
    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (firstTimeSetup)
    {
        float x = layout.marginX;
        float y = layout.menuBarHeight + layout.marginY;
        float width = windowWidth * layout.hierarchyWidthPercent - layout.marginX;
        float height = windowHeight * layout.hierarchyHeightPercent - layout.menuBarHeight - layout.marginY * 2;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (useAdaptiveLayout)
    {
        float x = layout.marginX;
        float y = layout.menuBarHeight + layout.marginY;
        float width = windowWidth * layout.hierarchyWidthPercent - layout.marginX;
        float height = windowHeight * layout.hierarchyHeightPercent - layout.menuBarHeight - layout.marginY * 2;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Hierarchy", &showHierarchy, ImGuiWindowFlags_HorizontalScrollbar);
    // Right-click on empty space to create empty GameObject
    if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty GameObject"))
        {
            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (opengl)
            {
                GameObject* emptyGO = new GameObject();
                int index = CountNames("GameObject_");
                emptyGO->name = "GameObject_" + std::to_string(index);

                // Set default transform
                emptyGO->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
                emptyGO->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                emptyGO->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

                // Set mesh index to -1 to indicate empty GameObject
                emptyGO->mesh->meshIndex = -1;
                emptyGO->meshPath = "";

                opengl->gameObjects.push_back(emptyGO);
                sceneModified = true;

                LOG("Created empty GameObject: " + emptyGO->name);
            }
        }
        ImGui::EndPopup();
    }

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (opengl)
    {
        // Draw root level GameObjects (those without parent)
        for (GameObject* go : opengl->gameObjects)
        {
            if (go != nullptr && go->parent == nullptr)
            {
                DrawGameObjectNode(go);
            }
        }
    }

    // Handle drag & drop to empty space (unparent)
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DRAG"))
        {
            GameObject* draggedGO = *(GameObject**)payload->Data;
            if (draggedGO != nullptr)
            {
                draggedGO->SetParent(nullptr);
                sceneModified = true;
                LOG("Unparented GameObject: " + draggedGO->name);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void ModuleEditor::DrawGameObjectNode(GameObject* go)
{
    if (go == nullptr || go->m_MarkedForDeletion)
        return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (IsSelected(go))
        flags |= ImGuiTreeNodeFlags_Selected;

    if (go->children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    std::string displayName = go->IsEmpty() ? "[E] " + go->name : go->name;

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)go->GetUUID(), flags, "%s", displayName.c_str());

    // Click: select with childs
    if (ImGui::IsItemClicked() && !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        SelectGameObject(go, true);
    }

    // Double click: select just that object
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        SelectGameObject(go, false);
        LOG("Double-clicked: Selecting only " + go->name);
    }

    // Context menu (right-click)
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Create Empty Child"))
        {
            GameObject* child = new GameObject(go);
            int index = CountNames("GameObject_");
            child->name = "GameObject_" + std::to_string(index);

            child->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
            child->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            child->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            child->mesh->meshIndex = -1;
            child->meshPath = "";

            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (opengl)
            {
                opengl->gameObjects.push_back(child);
                sceneModified = true;
                LOG("Created empty child GameObject: " + child->name);
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Move Up"))
        {
            go->MoveUp();
            sceneModified = true;
        }

        if (ImGui::MenuItem("Move Down"))
        {
            go->MoveDown();
            sceneModified = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
        {
            // Just mark for deletion - will be processed at end of frame
            MarkForDeletion(go);
        }

        ImGui::EndPopup();
    }

    // Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("GAMEOBJECT_DRAG", &go, sizeof(GameObject*));
        ImGui::Text("Move: %s", go->name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target
    HandleHierarchyDragDrop(go);

    // Draw children recursively
    if (nodeOpen)
    {
        // Create a copy to avoid issues
        std::vector<GameObject*> childrenCopy = go->children;

        for (GameObject* child : childrenCopy)
        {
            if (child && !child->m_MarkedForDeletion)
            {
                DrawGameObjectNode(child);
            }
        }
        ImGui::TreePop();
    }
}

void ModuleEditor::HandleHierarchyDragDrop(GameObject* target)
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DRAG"))
        {
            GameObject* draggedGO = *(GameObject**)payload->Data;

            if (draggedGO != nullptr && draggedGO != target)
            {
                // Check that we're not trying to parent to a descendant (would create cycle)
                bool isCyclic = false;
                GameObject* checkParent = target;
                while (checkParent != nullptr)
                {
                    if (checkParent == draggedGO)
                    {
                        isCyclic = true;
                        LOG_WARNING("Cannot parent GameObject to its own descendant!");
                        break;
                    }
                    checkParent = checkParent->parent;
                }

                if (!isCyclic)
                {
                    // Calculate the transform offset to maintain world position
                    // Get current world position
                    glm::vec3 worldPos(
                        draggedGO->transform->translation.x,
                        draggedGO->transform->translation.y,
                        draggedGO->transform->translation.z
                    );

                    // Set new parent
                    draggedGO->SetParent(target);

                    sceneModified = true;
                    LOG("Reparented " + draggedGO->name + " to " + target->name);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void ModuleEditor::DrawInspector()
{
    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (firstTimeSetup)
    {
        float x = windowWidth * layout.inspectorXPercent;
        float y = layout.menuBarHeight + layout.marginY;
        float width = windowWidth * layout.inspectorWidthPercent - layout.marginX;
        float height = windowHeight - layout.menuBarHeight - layout.marginY * 2;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (useAdaptiveLayout)
    {
        float x = windowWidth * layout.inspectorXPercent;
        float y = layout.menuBarHeight + layout.marginY;
        float width = windowWidth * layout.inspectorWidthPercent - layout.marginX;
        float height = windowHeight - layout.menuBarHeight - layout.marginY * 2;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Inspector", &showInspector);

    if (selectedGameObjects.empty())
    {
        ImGui::Text("No GameObject selected");
    }
    else if (selectedGameObjects.size() == 1)
    {
        // Show inspector for only one object (código existente)
        GameObject* selectedGameObject = selectedGameObjects[0];

        textureDropPos = ImGui::GetWindowPos();
        textureDropSize = ImGui::GetWindowSize();

        // editable name
        static GameObject* lastSelectedGO = nullptr;
        if (lastSelectedGO != selectedGameObject) {
            strncpy_s(nameBuffer, selectedGameObject->name.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            lastSelectedGO = selectedGameObject;
            editing = false;
        }

        ImGui::InputText("GameObject", nameBuffer, IM_ARRAYSIZE(nameBuffer));

        if (ImGui::IsItemActive()) editing = true;

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            editing = false;
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
                    sceneModified = true;
                }

                float scale[3] = { transform->scaling.x, transform->scaling.y, transform->scaling.z };
                if (ImGui::DragFloat3("Scale", scale, 0.01f, 0.01f, 100.0f))
                {
                    transform->scaling.x = scale[0];
                    transform->scaling.y = scale[1];
                    transform->scaling.z = scale[2];
                    sceneModified = true;
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
                if (updatedAngles) {
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

                    updatedAngles = false;
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
                    sceneModified = true;
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

                if (ImGui::Button("Use Checkerboard"))
                {
                    // Delete old texture
                    if (texture->texturedata->id != 0)
                        glDeleteTextures(1, &texture->texturedata->id);
                    AssignCheckerboardTexture(selectedGameObject);
                    LOG("Applied checkerboard texture to " + selectedGameObject->name);
                    selectedGameObject->texture->texturePath = "";
                }
            }
            else
            {
                ImGui::Text("No texture assigned");
            }

            //Drag and Drop Area for textures
            ImGui::Separator();
            ImGui::Text("Drag new texture in \ninspector tab to change it!");
        }
    }
    else
    {
        ImGui::Text("Multiple objects selected (%d)", (int)selectedGameObjects.size());
        ImGui::Separator();

        ImGui::Text("Selected GameObjects:");
        for (GameObject* go : selectedGameObjects)
        {
            ImGui::BulletText("%s", go->name.c_str());
        }

        ImGui::Separator();

        //Transform
        if (ImGui::CollapsingHeader("Multi-Object Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f),
                "Editing %d objects simultaneously", (int)selectedGameObjects.size());
            ImGui::Spacing();

            // Calculate average values
            glm::vec3 avgPosition(0.0f);
            glm::vec3 avgScale(0.0f);
            glm::vec3 avgRotation(0.0f);

            int validCount = 0;
            for (GameObject* go : selectedGameObjects)
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
                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        go->transform->translation.x += delta.x;
                        go->transform->translation.y += delta.y;
                        go->transform->translation.z += delta.z;
                    }
                }
                sceneModified = true;
                editing = true;
            }
            if (ImGui::IsItemDeactivated())
            {
                editing = false;
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
                for (GameObject* go : selectedGameObjects)
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
                sceneModified = true;
                editing = true;
            }
            if (ImGui::IsItemDeactivated())
            {
                editing = false;
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
                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        go->transform->scaling.x *= scaleFactor.x;
                        go->transform->scaling.y *= scaleFactor.y;
                        go->transform->scaling.z *= scaleFactor.z;
                    }
                }
                sceneModified = true;
                editing = true;
            }
            if (ImGui::IsItemDeactivated())
            {
                editing = false;
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // reset
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Quick Actions:");

            if (ImGui::Button("Reset Position", ImVec2(-1, 0)))
            {
                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        go->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
                    }
                }
                sceneModified = true;
                LOG("Reset position for " + std::to_string(selectedGameObjects.size()) + " objects");
            }

            if (ImGui::Button("Reset Rotation", ImVec2(-1, 0)))
            {
                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        go->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                    }
                }
                sceneModified = true;
                LOG("Reset rotation for " + std::to_string(selectedGameObjects.size()) + " objects");
            }

            if (ImGui::Button("Reset Scale", ImVec2(-1, 0)))
            {
                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        go->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);
                    }
                }
                sceneModified = true;
                LOG("Reset scale for " + std::to_string(selectedGameObjects.size()) + " objects");
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
    ImGui::End();
}

void ModuleEditor::DrawAbout()
{
    if (!showAbout) return;

    if (firstTimeSetup)
    {
        ImGui::SetNextWindowPos(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_Always);
    }

    ImGui::Begin("About", &showAbout);

    ImGui::Text("%s", motorName);
    ImGui::Text("Version: %s", version);
    ImGui::Separator();

    ImGui::Text("Team:");
    ImGui::BulletText("%s", team);
    ImGui::Separator();

    ImGui::Text("Libraries Used:");
    ImGui::BulletText("SDL3");
    ImGui::BulletText("OpenGL");
    ImGui::BulletText("GLAD");
    ImGui::BulletText("ImGui");
    ImGui::BulletText("DevIL");
    ImGui::BulletText("Assimp");
    ImGui::BulletText("GLM");
    ImGui::Separator();

    ImGui::Text("License: MIT License");
    ImGui::Separator();

    if (ImGui::Button("GitHub Repository"))
    {
        SDL_OpenURL(repoURL);
    }

    ImGui::End();
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

void ModuleEditor::DrawGuizmo()
{
    if (selectedGameObjects.empty())
        return;

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
        return;

    // Get camera matrix
    Camera* camera = &opengl->camera;
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    // If only one object, use its tranformation
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

        if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            currentGizmoOperation,
            currentGizmoMode,
            glm::value_ptr(model),
            glm::value_ptr(deltaMatrix)))
        {
            // Inicialize tracking if its first manipulation frame
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
            if (editing)
            {
                editing = false;
            }
        }
    }
    else
    {
        // Use center of selection when selecting multiple objects
        glm::vec3 selectionCenter = GetSelectionCenter();

        // Create identity matrix from selection center
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

            // Calculate new center
            glm::vec3 newCenter = glm::vec3(model[3]);

            if (currentGizmoOperation == ImGuizmo::TRANSLATE)
            {
                // Move all objects by the same center
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
                // Rotate all objects from center
                
                // Extract rotation from transformation matrix
                glm::vec3 dummyTranslation, dummyScale;
                glm::quat newRotation;
                glm::mat4 rotationMatrix = model;
                rotationMatrix[3] = glm::vec4(0, 0, 0, 1);
                
                // Decompose to get only rotation
                ComponentTransform tempTransform(nullptr);
                tempTransform.Decompose(model, dummyTranslation, newRotation, dummyScale);

                // Calculate delta rotation from last frame
                glm::quat deltaRotation = newRotation ;
                lastMultiSelectionRotation = newRotation;

                // Apply rotation from center
                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        // Current position
                        glm::vec3 objectPos(
                            go->transform->translation.x,
                            go->transform->translation.y,
                            go->transform->translation.z
                        );

                        // Vector from center
                        glm::vec3 offset = objectPos - selectionCenter;

                        glm::vec3 rotatedOffset = deltaRotation * offset;

                        // New position
                        glm::vec3 newPos = selectionCenter + rotatedOffset;

                        go->transform->translation.x = newPos.x;
                        go->transform->translation.y = newPos.y;
                        go->transform->translation.z = newPos.z;

                        // Rotate orentation
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
                // Scale from center

                // Extract current matrix scale
                glm::vec3 dummyTranslation, currentScale;
                glm::quat dummyRotation;
                ComponentTransform tempTransform(nullptr);
                tempTransform.Decompose(model, dummyTranslation, dummyRotation, currentScale);

                // Calculate scale factor relative to INITIAL scale
                glm::vec3 rawScaleFactor(1.0f);
                if (initialMultiSelectionScale.x > 0.0001f)
                    rawScaleFactor.x = currentScale.x / initialMultiSelectionScale.x;
                if (initialMultiSelectionScale.y > 0.0001f)
                    rawScaleFactor.y = currentScale.y / initialMultiSelectionScale.y;
                if (initialMultiSelectionScale.z > 0.0001f)
                    rawScaleFactor.z = currentScale.z / initialMultiSelectionScale.z;

                // Apply damping to make scaling smoother and prevent extreme values
                // Clamp the raw scale factor to a reasonable range first
                rawScaleFactor.x = glm::clamp(rawScaleFactor.x, 0.01f, 100.0f);
                rawScaleFactor.y = glm::clamp(rawScaleFactor.y, 0.01f, 100.0f);
                rawScaleFactor.z = glm::clamp(rawScaleFactor.z, 0.01f, 100.0f);

                // Calculate delta from last frame and apply smooth damping
                glm::vec3 deltaScale = rawScaleFactor / lastMultiSelectionScale;

                // Limit the rate of change per frame (prevents crazy jumps)
                deltaScale.x = glm::clamp(deltaScale.x, 0.5f, 2.0f);
                deltaScale.y = glm::clamp(deltaScale.y, 0.5f, 2.0f);
                deltaScale.z = glm::clamp(deltaScale.z, 0.5f, 2.0f);

                // Apply the smoothed delta
                glm::vec3 smoothedScaleFactor = lastMultiSelectionScale * deltaScale;
                lastMultiSelectionScale = smoothedScaleFactor;

                // Store original scales if this is the first frame
                static std::map<GameObject*, glm::vec3> originalScales;
                static std::map<GameObject*, glm::vec3> originalPositions;

                if (originalScales.empty())
                {
                    // Store original transforms
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

                // Apply scale from center
                for (GameObject* go : selectedGameObjects)
                {
                    if (go && go->transform)
                    {
                        // Get original position
                        glm::vec3 originalPos = originalPositions[go];

                        // Calculate offset from center
                        glm::vec3 offset = originalPos - selectionCenter;

                        // Scale the offset using smoothed factor
                        glm::vec3 scaledOffset = offset * smoothedScaleFactor;

                        // Set new position
                        glm::vec3 newPos = selectionCenter + scaledOffset;

                        go->transform->translation.x = newPos.x;
                        go->transform->translation.y = newPos.y;
                        go->transform->translation.z = newPos.z;

                        // Apply scale to object based on original scale
                        glm::vec3 originalScale = originalScales[go];
                        go->transform->scaling.x = originalScale.x * smoothedScaleFactor.x;
                        go->transform->scaling.y = originalScale.y * smoothedScaleFactor.y;
                        go->transform->scaling.z = originalScale.z * smoothedScaleFactor.z;

                        // Clamp final scale to prevent extreme values
                        go->transform->scaling.x = glm::clamp(go->transform->scaling.x, 0.001f, 1000.0f);
                        go->transform->scaling.y = glm::clamp(go->transform->scaling.y, 0.001f, 1000.0f);
                        go->transform->scaling.z = glm::clamp(go->transform->scaling.z, 0.001f, 1000.0f);
                    }
                }

                // Clear stored data when manipulation ends
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
            if (editing)
            {
                editing = false;
                
                // Reset acomulated scale/rotation
                lastMultiSelectionRotation = glm::quat(1, 0, 0, 0);
                lastMultiSelectionScale = glm::vec3(1, 1, 1);
                wasManipulating = false;
            }
        }
    }
}

int ModuleEditor::CountNames(std::string prefix)
{
    OpenGL* opengl = Application::GetInstance().opengl.get();
    int maxIndex = -1;

    for (size_t i = 0; i < opengl->gameObjects.size(); i++)
    {
        GameObject* obj = opengl->gameObjects[i];

        //check matches
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
            //get the numeric part
            std::string numberPart = obj->name.substr(prefix.size());
            int value = std::atoi(numberPart.c_str()); //from string to int

            if (value > maxIndex) maxIndex = value; //get the highest num
        }
    }

    return maxIndex + 1; //return the next num
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

    // load saved scene
    std::vector<GameObject*> loadedGameObjects;
    if (SceneSerializer::LoadScene(filepath, loadedGameObjects))
    {
        // Set loaded GameObjects
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

    // Clear selection first
    selectedGameObjects.clear();
    opengl->selectedGameObject = nullptr;

    // Breake childs/parents
    for (GameObject* go : opengl->gameObjects)
    {
        if (go != nullptr)
        {
            go->parent = nullptr;
            go->children.clear();
        }
    }

    // delete GameObjects
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

    // Clean previous selection
    selectedGameObjects.clear();

    // Add main GO
    selectedGameObjects.push_back(go);

    // Add descendants
    if (includeDescendants)
    {
        std::vector<GameObject*> descendants;
        go->GetAllDescendants(descendants);

        for (GameObject* descendant : descendants)
        {
            selectedGameObjects.push_back(descendant);
        }
    }

    // Update OpenGL selection
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

    // Collect all descendants
    std::vector<GameObject*> toMark;
    toMark.push_back(go);
    go->GetAllDescendants(toMark);

    // Mark all for deletion
    for (GameObject* obj : toMark)
    {
        if (obj && !obj->m_MarkedForDeletion)
        {
            obj->m_MarkedForDeletion = true;

            // Add to deletion queue if not already there
            if (std::find(m_ObjectsToDelete.begin(), m_ObjectsToDelete.end(), obj) == m_ObjectsToDelete.end())
            {
                m_ObjectsToDelete.push_back(obj);
            }
        }
    }

    // Deselect if selected
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

    // Identify root objects (those without parent or whose parent is also being deleted)
    std::vector<GameObject*> rootsToDelete;

    for (GameObject* go : m_ObjectsToDelete)
    {
        if (!go) continue;

        bool isRoot = (go->parent == nullptr);

        // Or parent is not marked for deletion
        if (go->parent != nullptr)
        {
            if (std::find(m_ObjectsToDelete.begin(), m_ObjectsToDelete.end(), go->parent) == m_ObjectsToDelete.end())
            {
                // Parent exists but is NOT being deleted, so this is effectively a root
                isRoot = true;
            }
        }

        if (isRoot)
        {
            rootsToDelete.push_back(go);
        }
    }

    // Remove ALL marked objects from gameObjects list FIRST
    for (GameObject* go : m_ObjectsToDelete)
    {
        auto it = std::find(opengl->gameObjects.begin(), opengl->gameObjects.end(), go);
        if (it != opengl->gameObjects.end())
        {
            opengl->gameObjects.erase(it);
        }

        // Unlink from parent
        if (go->parent != nullptr)
        {
            go->parent->RemoveChild(go);
            go->parent = nullptr;
        }
    }

    // Now delete only the roots - their destructors will handle children
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