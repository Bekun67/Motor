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
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


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

            // Generate the cube mesh and get its index
            int meshIndex = PrimitiveGenerator::GenerateCube(2.0f);

            // Create GameObject
            GameObject* cube = new GameObject();
            cube->name = "Cube_" + std::to_string(Application::GetInstance().opengl->gameObjects.size());

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

            LOG("Created GameObject: " + cube->name + " with meshIndex: " + std::to_string(meshIndex));
            LOG("Total GameObjects in scene: " + std::to_string(Application::GetInstance().opengl->gameObjects.size()));
        }

        if (ImGui::Button("Create Sphere"))
        {
            LOG("Creating sphere primitive");

            // Generate the sphere mesh and get its index
            int meshIndex = PrimitiveGenerator::GenerateSphere(1.0f, 32, 16);

            // Create GameObject
            GameObject* sphere = new GameObject();
            sphere->name = "Sphere_" + std::to_string(Application::GetInstance().opengl->gameObjects.size());

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

            LOG("Created GameObject: " + sphere->name + " with meshIndex: " + std::to_string(meshIndex));
            LOG("Total GameObjects in scene: " + std::to_string(Application::GetInstance().opengl->gameObjects.size()));
        }

        if (ImGui::Button("Create Cylinder"))
        {
            LOG("Creating cylinder primitive");

            // Generate the cylinder mesh and get its index
            int meshIndex = PrimitiveGenerator::GenerateCylinder(0.5f, 2.0f, 32);

            // Create GameObject
            GameObject* cylinder = new GameObject();
            cylinder->name = "Cylinder_" + std::to_string(Application::GetInstance().opengl->gameObjects.size());

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

            LOG("Created GameObject: " + cylinder->name + " with meshIndex: " + std::to_string(meshIndex));
            LOG("Total GameObjects in scene: " + std::to_string(Application::GetInstance().opengl->gameObjects.size()));
        }

        if (ImGui::Button("Create Plane"))
        {
            LOG("Creating plane primitive");

            // Generate the plane mesh and get its index
            int meshIndex = PrimitiveGenerator::GeneratePlane(5.0f, 5.0f, 10, 10);

            // Create GameObject
            GameObject* plane = new GameObject();
            plane->name = "Plane_" + std::to_string(Application::GetInstance().opengl->gameObjects.size());

            // Set transform
            plane->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
            plane->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            plane->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            // Assign mesh index
            plane->mesh->meshIndex = meshIndex;

            // Assign checkerboard texture
            AssignCheckerboardTexture(plane);

            // Add to gameObjects list
            Application::GetInstance().opengl->gameObjects.push_back(plane);

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
                }

                float scale[3] = { transform->scaling.x, transform->scaling.y, transform->scaling.z };
                if (ImGui::DragFloat3("Scale", scale, 0.01f, 0.01f, 100.0f))
                {
                    transform->scaling.x = scale[0];
                    transform->scaling.y = scale[1];
                    transform->scaling.z = scale[2];
                }

                //rotation
                static glm::vec3 rotationEuler = glm::vec3(0.0f); 
                static float lastAngles[3] = { 0.0f, 0.0f, 0.0f };
                static bool firstFrame = true;

                // first rotation
                if (firstFrame)
                {
                    glm::quat q(transform->rotation.w, transform->rotation.x, transform->rotation.y, transform->rotation.z);
                    rotationEuler = glm::degrees(glm::eulerAngles(q));
                    lastAngles[0] = rotationEuler.x;
                    lastAngles[1] = rotationEuler.y;
                    lastAngles[2] = rotationEuler.z;
                    firstFrame = false;
                }
                if (updatedAngles) {
                    glm::quat q(transform->rotation.w, transform->rotation.x, transform->rotation.y, transform->rotation.z);
                    rotationEuler = glm::degrees(glm::eulerAngles(q));
                    updatedAngles = false;
                }
                // edit tab
                float angles[3] = { rotationEuler.x, rotationEuler.y, rotationEuler.z };
                ImGui::InputFloat3("Rotation", angles, "%.2f");

                // detect if the value has changed
                bool changed = false;
                for (int i = 0; i < 3; ++i)
                {
                    if (angles[i] != lastAngles[i])
                    {
                        changed = true;
                        lastAngles[i] = angles[i]; 
                    }
                }

                //if it has changed we apply it
                if (changed)
                {
                    rotationEuler = glm::vec3(angles[0], angles[1], angles[2]);

                    glm::quat newQuat = glm::quat(glm::radians(rotationEuler));
                    transform->rotation.w = newQuat.w;
                    transform->rotation.x = newQuat.x;
                    transform->rotation.y = newQuat.y;
                    transform->rotation.z = newQuat.z;
                }

                // show quat
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