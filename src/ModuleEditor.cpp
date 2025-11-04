#include "ModuleEditor.h"
#include "Application.h"
#include "OpenGL.h"
#include "Window.h"
#include "Input.h"
#include "Texture.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "ComponentTransform.h"
#include "LoadFBX.h"
#include "PrimitiveGenerator.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <IL/il.h>
#include <iostream>
#include <string>

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

    // Draw all editor windows
    DrawMenuBar();

    if (showConsole) DrawConsole();
    if (showConfiguration) DrawConfiguration();
    if (showHierarchy) DrawHierarchy();
    if (showInspector) DrawInspector();
    if (showAbout) DrawAbout();

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

void ModuleEditor::DrawMenuBar()
{
    ImGui::Begin("Main Menu");

    if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Exit"))
        {
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
            int meshIndex = PrimitiveGenerator::GenerateCube(2.0f);

            GameObject* cube = new GameObject();
            cube->name = "Cube_" + std::to_string(Application::GetInstance().opengl->gameObjects.size());
            cube->transform->translation = aiVector3D(0.0f, 1.0f, 0.0f);
            cube->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            cube->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);
            cube->mesh->meshIndex = meshIndex;
            AssignCheckerboardTexture(cube);

            Application::GetInstance().opengl->gameObjects.push_back(cube);
            std::cout << "Created GameObject " << cube->name << std::endl;
        }
        if (ImGui::Button("Create Sphere"))
        {
            LOG("Creating sphere primitive (TODO)");
        }
        if (ImGui::Button("Create Cilinder"))
        {
            LOG("Creating cilinder primitive (TODO)");
        }
        if (ImGui::Button("Create Plane"))
        {
            LOG("Creating plane primitive (TODO)");
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
            //not yet implemented
            std::string url = std::string(repoURL) ;
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
}

void ModuleEditor::DrawConsole()
{
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
    ImGui::Begin("Configuration", &showConfiguration);

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
    ImGui::Begin("Hierarchy", &showHierarchy);

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
    ImGui::Begin("Inspector", &showInspector);

    if (selectedGameObject == nullptr)
    {
        ImGui::Text("No GameObject selected");
    }
    else
    {
        ImGui::Text("GameObject: %s", selectedGameObject->name.c_str());
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
                }

                float scale[3] = { transform->scaling.x, transform->scaling.y, transform->scaling.z };
                if (ImGui::DragFloat3("Scale", scale, 0.01f, 0.01f, 100.0f))
                {
                    transform->scaling.x = scale[0];
                    transform->scaling.y = scale[1];
                    transform->scaling.z = scale[2];
                }

                //TODO hacer bien la rotacion hacer editables unos angulos
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
                ImGui::Text("Vertices: %d", meshData.numIndices);
                ImGui::Text("VAO: %u", meshData.VAO);
                ImGui::Text("VBO: %u", meshData.VBO);
                ImGui::Text("EBO: %u", meshData.EBO);

                // TODO: Add options for showing normals
                static bool showVertexNormals = false;
                static bool showFaceNormals = false;
                ImGui::Checkbox("Show Vertex Normals", &showVertexNormals);
                ImGui::Checkbox("Show Face Normals", &showFaceNormals);
            }
            else
            {
                ImGui::Text("No mesh assigned");
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
                //TODO enseñar textura
                //ImGui::Image((void*)(intptr_t)texture->texturedata->id, ImVec2(128, 128));

                if (ImGui::Button("Use Checkerboard"))
                {
                    // Delete old texture
                    if (texture->texturedata->id != 0)
                        glDeleteTextures(1, &texture->texturedata->id);
                    AssignCheckerboardTexture(selectedGameObject);
                    LOG("Applied checkerboard texture to " + selectedGameObject->name);
                }
            }
            else
            {
                ImGui::Text("No texture assigned");
            }
        }
    }

    ImGui::End();
}

void ModuleEditor::DrawAbout()
{
    if (!showAbout) return;

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