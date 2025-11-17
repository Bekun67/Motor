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

    // Draw Gizmo (debe ser lo �ltimo)
    DrawGuizmo();

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
        float x = windowWidth * layout.sceneXPercent + layout.marginX;
        float y = layout.marginY;
        float width = windowWidth * layout.sceneWidthPercent - layout.marginX;
        float height = windowHeight * layout.sceneHeightPercent;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (useAdaptiveLayout)
    {
        float x = windowWidth * layout.sceneXPercent + layout.marginX;
        float y = layout.marginY;
        float width = windowWidth * layout.sceneWidthPercent - layout.marginX;
        float height = windowHeight * layout.sceneHeightPercent;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

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

    if (firstTimeSetup)
    {
        float width = windowWidth * layout.menuWidthPercent;
        float height = windowHeight * layout.menuHeightPercent;

        ImGui::SetNextWindowPos(ImVec2(layout.marginX, layout.marginY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

        // Disable first time setup after first frame
        if (ImGui::GetFrameCount() > 2)
        {
            firstTimeSetup = false;
        }
    }
    else if (useAdaptiveLayout)
    {
        float width = windowWidth * layout.menuWidthPercent;
        float height = windowHeight * layout.menuHeightPercent;

        ImGui::SetNextWindowPos(ImVec2(layout.marginX, layout.marginY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Main Menu");

    if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("New Scene"))
        {
            if (sceneModified && !currentScenePath.empty())
            {
                LOG_WARNING("Current scene has unsaved changes!");
            }

            // Clear current scene
            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (opengl)
            {
                for (GameObject* go : opengl->gameObjects)
                {
                    delete go;
                }
                opengl->gameObjects.clear();
                selectedGameObject = nullptr;
                opengl->selectedGameObject = nullptr;

                currentScenePath = "";
                sceneModified = false;
                LOG("New scene created");
            }
        }

        if (ImGui::Button("Save Scene"))
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

        if (ImGui::Button("Save Scene As..."))
        {
            SaveSceneDialog();
        }

        if (ImGui::Button("Load Scene"))
        {
            LoadSceneDialog();
        }

        ImGui::Separator();

        if (ImGui::Button("Exit"))
        {
            if (sceneModified)
            {
                LOG_WARNING("Scene has unsaved changes!");
            }

            SDL_Event quitEvent;
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        }
    }

    if (ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Console", &showConsole);
        ImGui::Checkbox("Configuration", &showConfiguration);
        ImGui::Checkbox("Hierarchy", &showHierarchy);
        ImGui::Checkbox("Inspector", &showInspector);
    }

    if (ImGui::CollapsingHeader("GameObject"))
    {
        if (ImGui::Button("Create Cube"))
        {
            LOG("Creating cube primitive");

            float size = 2.0f;
            // Generate the cube mesh and get its index
            int meshIndex = PrimitiveGenerator::GenerateCube(size);

            // Create GameObject
            GameObject* cube = new GameObject();

            int index = CountNames("Cube_");
            cube->name = "Cube_" + std::to_string(index);
            cube->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::CUBE, size, 0, 0, 0);
            cube->meshIndexInFBX = 0; 

            // Set transform
            cube->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
            cube->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            cube->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            // Assign mesh index
            cube->mesh->meshIndex = meshIndex;

            // Assign checkerboard texture
            AssignCheckerboardTexture(cube);

            // Add to gameObjects list
            Application::GetInstance().opengl->gameObjects.push_back(cube);

            sceneModified = true;

            LOG("Created GameObject: " + cube->name + " with meshIndex: " + std::to_string(meshIndex));
            LOG("Total GameObjects in scene: " + std::to_string(Application::GetInstance().opengl->gameObjects.size()));
        }

        if (ImGui::Button("Create Sphere"))
        {
            LOG("Creating sphere primitive");

            float radius = 1.0f;
            int segments = 32;
            int rings = 16;

            // Generate the sphere mesh and get its index
            int meshIndex = PrimitiveGenerator::GenerateSphere(radius, segments, rings);

            // Create GameObject
            GameObject* sphere = new GameObject();

            int index = CountNames("Sphere_");
            sphere->name = "Sphere_" + std::to_string(index);
            sphere->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::SPHERE, radius, 0, segments, rings);
            sphere->meshIndexInFBX = 0;

            // Set transform
            sphere->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
            sphere->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            sphere->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            // Assign mesh index
            sphere->mesh->meshIndex = meshIndex;

            // Assign checkerboard texture
            AssignCheckerboardTexture(sphere);

            // Add to gameObjects list
            Application::GetInstance().opengl->gameObjects.push_back(sphere);

            sceneModified = true;

            LOG("Created GameObject: " + sphere->name + " with meshIndex: " + std::to_string(meshIndex));
            LOG("Total GameObjects in scene: " + std::to_string(Application::GetInstance().opengl->gameObjects.size()));
        }

        if (ImGui::Button("Create Cylinder"))
        {
            LOG("Creating cylinder primitive");

            float radius = 0.5f;
            float height = 2.0f;
            int segments = 32;

            // Generate the cylinder mesh and get its index
            int meshIndex = PrimitiveGenerator::GenerateCylinder(radius, height, segments);

            // Create GameObject
            GameObject* cylinder = new GameObject();

            int index = CountNames("Cylinder_");
            cylinder->name = "Cylinder_" + std::to_string(index);
            cylinder->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::CYLINDER, radius, height, segments, 0);
            cylinder->meshIndexInFBX = 0;

            // Set transform
            cylinder->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
            cylinder->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            cylinder->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            // Assign mesh index
            cylinder->mesh->meshIndex = meshIndex;

            // Assign checkerboard texture
            AssignCheckerboardTexture(cylinder);

            // Add to gameObjects list
            Application::GetInstance().opengl->gameObjects.push_back(cylinder);

            sceneModified = true;

            LOG("Created GameObject: " + cylinder->name + " with meshIndex: " + std::to_string(meshIndex));
            LOG("Total GameObjects in scene: " + std::to_string(Application::GetInstance().opengl->gameObjects.size()));
        }

        if (ImGui::Button("Create Plane"))
        {
            LOG("Creating plane primitive");

            float width = 5.0f;
            float depth = 5.0f;
            int widthSegments = 10;
            int depthSegments = 10;

            // Generate the plane mesh and get its index
            int meshIndex = PrimitiveGenerator::GeneratePlane(width, depth, widthSegments, depthSegments);

            // Create GameObject
            GameObject* plane = new GameObject();

            int index = CountNames("Plane_");
            plane->name = "Plane_" + std::to_string(index);

            // Set transform
            plane->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
            plane->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            plane->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);
            plane->meshPath = PrimitiveGenerator::GetPrimitiveMeshPath(PrimitiveType::PLANE, width, depth, widthSegments, depthSegments);
            plane->meshIndexInFBX = 0;

            // Assign mesh index
            plane->mesh->meshIndex = meshIndex;

            // Assign checkerboard texture
            AssignCheckerboardTexture(plane);

            // Add to gameObjects list
            Application::GetInstance().opengl->gameObjects.push_back(plane);

            sceneModified = true;

            LOG("Created GameObject: " + plane->name + " with meshIndex: " + std::to_string(meshIndex));
            LOG("Total GameObjects in scene: " + std::to_string(Application::GetInstance().opengl->gameObjects.size()));
        }
    }

    if (ImGui::CollapsingHeader("Help"))
    {
        if (ImGui::Button("Documentation"))
        {
            std::string url = std::string(repoURL) + "/blob/main/README.md";
            SDL_OpenURL(url.c_str());
            LOG("Opening documentation: " + url);
        }
        if (ImGui::Button("Report a Bug"))
        {
            std::string url = std::string(repoURL);
            SDL_OpenURL(url.c_str());
            LOG("Opening issues page: " + url);
        }
        if (ImGui::Button("Download Latest"))
        {
            std::string url = std::string(repoURL) + "/releases";
            SDL_OpenURL(url.c_str());
            LOG("Opening releases page: " + url);
        }
        ImGui::Separator();
        if (ImGui::Button("About"))
        {
            showAbout = true;
        }
    }

    ImGui::End();

    // Save Scene Dialog
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

    // Load Scene Dialog
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
                    std::string filepath = FileSystemManager::GetScenesDirectory() + sceneName;
                    LoadScene(filepath);
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
        float width = windowWidth * layout.consoleWidthPercent - layout.marginX;
        float height = windowHeight * layout.consoleHeightPercent;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (useAdaptiveLayout)
    {
        float x = windowWidth * layout.consoleXPercent + layout.marginX;
        float y = windowHeight * layout.consoleYPercent;
        float width = windowWidth * layout.consoleWidthPercent - layout.marginX;
        float height = windowHeight * layout.consoleHeightPercent;

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
        // Posicionar en la parte superior derecha
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
        float y = windowHeight * layout.hierarchyYPercent;
        float width = windowWidth * layout.hierarchyWidthPercent;
        float height = windowHeight * layout.hierarchyHeightPercent;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (useAdaptiveLayout)
    {
        float x = layout.marginX;
        float y = windowHeight * layout.hierarchyYPercent;
        float width = windowWidth * layout.hierarchyWidthPercent;
        float height = windowHeight * layout.hierarchyHeightPercent;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Hierarchy", &showHierarchy, ImGuiWindowFlags_HorizontalScrollbar);

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (opengl)
    {
        for (size_t i = 0; i < opengl->gameObjects.size(); ++i)
        {
            GameObject* go = opengl->gameObjects[i];
            if (go)
            {
                bool isSelected = (go == selectedGameObject);
                if (ImGui::Selectable(go->name.c_str(), isSelected))
                {
                    selectedGameObject = go;
                    opengl->selectedGameObject = go;
                    LOG("Selected GameObject: " + go->name);
                }
            }
        }
    }

    ImGui::End();
}

void ModuleEditor::DrawInspector()
{

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (firstTimeSetup)
    {
        float x = windowWidth * layout.inspectorXPercent;
        float y = layout.marginY;
        float width = windowWidth * layout.inspectorWidthPercent;
        float height = windowHeight * layout.inspectorHeightPercent - layout.marginY;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (useAdaptiveLayout)
    {
        float x = windowWidth * layout.inspectorXPercent;
        float y = layout.marginY;
        float width = windowWidth * layout.inspectorWidthPercent;
        float height = windowHeight * layout.inspectorHeightPercent - layout.marginY;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Inspector", &showInspector);

    if (selectedGameObject == nullptr)
    {
        ImGui::Text("No GameObject selected");
    }
    else
    {
        //editable name
        static GameObject* lastSelectedGO = nullptr;
        if (lastSelectedGO != selectedGameObject) {
            strncpy_s(nameBuffer, selectedGameObject->name.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            lastSelectedGO = selectedGameObject;
            editing = false;
        }

        ImGui::InputText("GameObject", nameBuffer, IM_ARRAYSIZE(nameBuffer));

        //we do this to prevent using keyboard controls while editing the name
        if (ImGui::IsItemActive()) editing = true;

        //when we deselect the name input
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            //if we have modified the name input we change de object's name
            editing = false;
            if (strlen(nameBuffer) > 0) {
                selectedGameObject->name = std::string(nameBuffer);
            }
            else {
                //if the name is "" we revert, as if we don't do this the engine crashes
                strncpy_s(nameBuffer, selectedGameObject->name.c_str(), sizeof(nameBuffer) - 1);
                nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            }
        }
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
                //showing data
                MeshData& meshData = g_Meshes[mesh->meshIndex];
                ImGui::Text("AABB (Local Space)");
                ImGui::Text("Min: (%.2f, %.2f, %.2f)", meshData.aabbMin.x, meshData.aabbMin.y, meshData.aabbMin.z);
                ImGui::Text("Max: (%.2f, %.2f, %.2f)", meshData.aabbMax.x, meshData.aabbMax.y, meshData.aabbMax.z);

                glm::vec3 center = (meshData.aabbMin + meshData.aabbMax) * 0.5f;
                glm::vec3 size = meshData.aabbMax - meshData.aabbMin;
                ImGui::Text("Center: (%.2f, %.2f, %.2f)", center.x, center.y, center.z);
                ImGui::Text("Size: (%.2f, %.2f, %.2f)", size.x, size.y, size.z);

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
                ImGui::Text("No boundig box assigned");
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

            //Drag adn Drop Area for textures
            ImGui::Separator();
            ImGui::Text("Drag new texture here:");

            //we change isMouseOverTextureDropZone, and input.cpp uses this to drop the texture there
            ImVec2 dropZoneSize(256, 28);
            ImGui::Button(" ", dropZoneSize);

            isMouseOverTextureDropZone = ImGui::IsItemHovered();

            textureDropZoneMin = ImGui::GetItemRectMin();
            textureDropZoneMax = ImGui::GetItemRectMax();
        }
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
    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl || !opengl->selectedGameObject)
        return;

    GameObject* selected = opengl->selectedGameObject;
    if (!selected->transform)
        return;

    // Get camera matrices
    Camera* camera = &opengl->camera;
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();

    // Create model matrix from transform
    glm::mat4 model = glm::mat4(1.0f);

    // Apply translation
    model = glm::translate(model, glm::vec3(
        selected->transform->translation.x,
        selected->transform->translation.y,
        selected->transform->translation.z
    ));

    // Apply rotation
    glm::quat quat(
        selected->transform->rotation.w,
        selected->transform->rotation.x,
        selected->transform->rotation.y,
        selected->transform->rotation.z
    );
    model *= glm::mat4_cast(quat);

    // Apply scale
    model = glm::scale(model, glm::vec3(
        selected->transform->scaling.x,
        selected->transform->scaling.y,
        selected->transform->scaling.z
    ));

    // Set ImGuizmo rect to full window
    ImGuizmo::SetRect(
        sceneViewportPos.x,
        sceneViewportPos.y,
        sceneViewportSize.x,
        sceneViewportSize.y
    );

    // Draw and manipulate
    glm::mat4 deltaMatrix = glm::mat4(1.0f);

    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        currentGizmoOperation,
        currentGizmoMode,
        glm::value_ptr(model),
        glm::value_ptr(deltaMatrix)))
    {
		// If we are manipulating
        editing = true;

        // Decompose the model matrix back to transform components
        glm::vec3 newTranslation, newScale;
        glm::quat newRotation;

        // Decompose matrix
        selected->transform->Decompose(model, newTranslation, newRotation, newScale);

        // Update transform
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
		// Not using the Guizmo
        if (editing)
        {
            editing = false;
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

    // Ask user to save current scene if modified
    if (sceneModified && !currentScenePath.empty())
    {
        // In a real implementation, you would show a dialog here
        LOG_WARNING("Current scene has unsaved changes");
    }

    std::vector<GameObject*> loadedGameObjects;
    if (SceneSerializer::LoadScene(filepath, loadedGameObjects))
    {
        // Clear current scene
        for (GameObject* go : opengl->gameObjects)
        {
            delete go;
        }
        opengl->gameObjects.clear();

        // Set loaded GameObjects
        opengl->gameObjects = loadedGameObjects;

        // Clear selection
        selectedGameObject = nullptr;
        opengl->selectedGameObject = nullptr;

        currentScenePath = filepath;
        sceneModified = false;

        LOG("Scene loaded successfully: " + filepath);
        return true;
    }

    LOG_ERROR("Failed to load scene: " + filepath);
    return false;
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