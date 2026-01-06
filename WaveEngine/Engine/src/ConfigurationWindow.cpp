#include "ConfigurationWindow.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <IL/il.h>
#include <windows.h>
#include <psapi.h>
#include "Application.h"
#include "Log.h"

ConfigurationWindow::ConfigurationWindow()
    : EditorWindow("Configuration")
{
    fpsHistory.reserve(maxFPSHistory);
}

void ConfigurationWindow::Draw()
{
    if (!isOpen) return;

    ImGui::Begin(name.c_str(), &isOpen);

    isHovered = (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows));

    if (ImGui::CollapsingHeader("FPS"))
    {
        DrawFPSGraph();
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Window"))
    {
        DrawWindowSettings();
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Camera"))
    {
        DrawCameraSettings();
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Renderer"))
    {
        DrawRendererSettings();
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Physics"))
    {
        DrawPhysicsSettings();
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Hardware"))
    {
        DrawHardwareInfo();
    }

    ImGui::End();
}

void ConfigurationWindow::DrawFPSGraph()
{
    fpsTimer += ImGui::GetIO().DeltaTime;

    if (fpsTimer >= 0.1f)
    {
        float fps = ImGui::GetIO().Framerate;
        fpsHistory.push_back(fps);

        if (fpsHistory.size() > maxFPSHistory)
        {
            fpsHistory.erase(fpsHistory.begin());
        }

        fpsTimer = 0.0f;
    }

    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

    if (!fpsHistory.empty())
    {
        ImGui::PlotLines("##FPS", fpsHistory.data(), (int)fpsHistory.size(), 0, nullptr, 0.0f, 200.0f, ImVec2(0, 80));
    }
}

void ConfigurationWindow::DrawHardwareInfo()
{
    ImGui::Text("CPU Cores: %d", SDL_GetNumLogicalCPUCores());
    ImGui::Text("System RAM: %d MB", SDL_GetSystemRAM());

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        SIZE_T memoryUsedMB = pmc.WorkingSetSize / (1024 * 1024);
        ImGui::Text("Memory Usage: %llu MB", memoryUsedMB);
    }
#endif

    ImGui::Text("GPU: %s", glGetString(GL_RENDERER));

    ImGui::Separator();
    ImGui::Text("Software Versions:");

    int sdlVersion = SDL_GetVersion();
    int major = SDL_VERSIONNUM_MAJOR(sdlVersion);
    int minor = SDL_VERSIONNUM_MINOR(sdlVersion);
    int patch = SDL_VERSIONNUM_MICRO(sdlVersion);
    ImGui::BulletText("SDL3: %d.%d.%d", major, minor, patch);

    ImGui::BulletText("OpenGL: %s", glGetString(GL_VERSION));
    ImGui::BulletText("ImGui: %s", IMGUI_VERSION);

    ILint devilVersion = ilGetInteger(IL_VERSION_NUM);
    int devilMajor = devilVersion / 100;
    int devilMinor = (devilVersion / 10) % 10;
    int devilPatch = devilVersion % 10;
    ImGui::BulletText("DevIL: %d.%d.%d", devilMajor, devilMinor, devilPatch);
}

void ConfigurationWindow::DrawWindowSettings()
{
    int width, height;
    Application::GetInstance().window->GetWindowSize(width, height);

    ImGui::Text("Window Size: %dx%d", width, height);

    int tempWidth = width;
    if (ImGui::InputInt("Width", &tempWidth, 10, 100))
    {
        if (tempWidth > 0)
        {
            SDL_SetWindowSize(Application::GetInstance().window->GetWindow(), tempWidth, height);
        }
    }

    int tempHeight = height;
    if (ImGui::InputInt("Height", &tempHeight, 10, 100))
    {
        if (tempHeight > 0)
        {
            SDL_SetWindowSize(Application::GetInstance().window->GetWindow(), width, tempHeight);
        }
    }

    ImGui::Separator();

    if (ImGui::Checkbox("Fullscreen", &fullscreen))
    {
        SDL_SetWindowFullscreen(Application::GetInstance().window->GetWindow(), fullscreen);
    }

    static bool borderlessFullscreen = false;
    if (ImGui::Checkbox("Borderless Window", &borderlessFullscreen))
    {
        SDL_SetWindowBordered(Application::GetInstance().window->GetWindow(), !borderlessFullscreen);
    }

    ImGui::Separator();

    static bool resizable = true;
    if (ImGui::Checkbox("Resizable", &resizable))
    {
        SDL_SetWindowResizable(Application::GetInstance().window->GetWindow(), resizable);
    }

    ImGui::Separator();

    ImGui::Text("Resolutions:");

    if (ImGui::Button("Set 1920x1080"))
    {
        SDL_SetWindowSize(Application::GetInstance().window->GetWindow(), 1920, 1080);
    }
    ImGui::SameLine();
    if (ImGui::Button("Set 1280x720"))
    {
        SDL_SetWindowSize(Application::GetInstance().window->GetWindow(), 1280, 720);
    }
}

void ConfigurationWindow::DrawCameraSettings()
{
    Application& app = Application::GetInstance();

    ImGui::Text("Camera Controls");
    ImGui::Separator();

    bool usingEditorCamera = app.camera->IsUsingEditorCamera();
    ImGui::Text("Active Camera Mode:");
    if (ImGui::RadioButton("Editor Camera", usingEditorCamera))
    {
        app.camera->SetUseEditorCamera(true);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Scene Camera", !usingEditorCamera))
    {
        if (app.camera->GetSceneCamera() != nullptr)
        {
            app.camera->SetUseEditorCamera(false);
        }
        else
        {
            LOG_CONSOLE("WARNING: No Scene Camera found. Create one from Camera menu.");
        }
    }
    ImGui::Separator();

    ComponentCamera* camera = app.renderer->GetCamera();

    if (camera == nullptr)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Camera not available");
        return;
    }

    if (usingEditorCamera)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active: Editor Camera");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "Active: Scene Camera");
    }

    ImGui::Separator();

    cameraMouseSensitivity = camera->GetMouseSensitivity();
    cameraScrollSpeed = camera->GetScrollSpeed();
    cameraPanSensitivity = camera->GetPanSensitivity();
    cameraMovementSpeed = camera->GetMovementSpeed();
    cameraFOV = camera->GetFov();

    ImGui::PushItemWidth(80);

    if (ImGui::SliderFloat("Movement Speed", &cameraMovementSpeed, 0.1f, 10.0f, "%.2f"))
    {
        camera->SetMovementSpeed(cameraMovementSpeed);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls how fast the camera moves (WASD keys)");

    if (ImGui::SliderFloat("Mouse Sensitivity", &cameraMouseSensitivity, 0.01f, 1.0f, "%.3f"))
    {
        camera->SetMouseSensitivity(cameraMouseSensitivity);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls camera rotation sensitivity when right-clicking");

    if (ImGui::SliderFloat("Scroll Speed", &cameraScrollSpeed, 0.1f, 2.0f, "%.2f"))
    {
        camera->SetScrollSpeed(cameraScrollSpeed);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls zoom speed with mouse wheel");

    if (ImGui::SliderFloat("Pan Sensitivity", &cameraPanSensitivity, 0.001f, 0.01f, "%.4f"))
    {
        camera->SetPanSensitivity(cameraPanSensitivity);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls camera panning sensitivity (middle mouse button)");

    ImGui::Spacing();

    if (ImGui::SliderFloat("Field of View", &cameraFOV, 20.0f, 120.0f, "%.1f"))
    {
        camera->SetFov(cameraFOV);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Camera field of view in degrees");

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("Reset to Defaults"))
    {
        cameraMovementSpeed = 2.5f;
        cameraMouseSensitivity = 0.2f;
        cameraScrollSpeed = 0.5f;
        cameraFOV = 45.0f;
        cameraPanSensitivity = 0.003f;

        camera->SetMovementSpeed(cameraMovementSpeed);
        camera->SetMouseSensitivity(cameraMouseSensitivity);
        camera->SetScrollSpeed(cameraScrollSpeed);
        camera->SetFov(cameraFOV);
        camera->SetPanSensitivity(cameraPanSensitivity);

        LOG_CONSOLE("Camera settings reset to defaults");
        LOG_DEBUG("Camera settings reset to defaults");
    }

    ImGui::Spacing();

    glm::vec3 camPos = camera->GetPosition();
    ImGui::Text("Camera Position:");
    ImGui::BulletText("X: %.2f  Y: %.2f  Z: %.2f", camPos.x, camPos.y, camPos.z);

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Camera Controls")) {
        ImGui::Text("Camera Controls:");
        ImGui::BulletText("Right Click + Drag: Rotate camera");
        ImGui::BulletText("Middle Click + Drag: Pan camera");
        ImGui::BulletText("Mouse Wheel: Zoom in/out");
        ImGui::BulletText("Alt + Right Click: Orbit around target");
        ImGui::BulletText("F: Focus on selected object");
    }
    ImGui::Separator();

    ComponentCamera* renderCam = app.renderer->GetCamera();
    ComponentCamera* sceneCam = app.camera->GetSceneCamera();

    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Render Camera:");
    ImGui::SameLine();
    ImGui::Text("%s", app.camera->IsUsingEditorCamera() ? "Editor Camera" : "Scene Camera");

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Culling Camera:");
    ImGui::SameLine();
    if (sceneCam)
    {
        ImGui::Text("Scene Camera");
        if (sceneCam->IsFrustumCullingEnabled())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "? ACTIVE");
        }
        else
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "? DISABLED");
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "None (create one)");
    }

    ImGui::Separator();
}

void ConfigurationWindow::DrawRendererSettings()
{
    ImGui::Text("OpenGL Renderer Settings");
    ImGui::Separator();

    Renderer* renderer = Application::GetInstance().renderer.get();
    if (renderer == nullptr)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Renderer not available");
        return;
    }

    if (ImGui::Checkbox("Show Z-Buffer", &showZBuffer))
    {
        Application::GetInstance().renderer->SetShowZBuffer(showZBuffer);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Visualize depth buffer as grayscale\nWhite = Near, Black = Far");
    }
	ImGui::Spacing();

    bool faceCulling = renderer->IsFaceCullingEnabled();
    if (ImGui::Checkbox("Face Culling", &faceCulling))
    {
        renderer->SetFaceCulling(faceCulling);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enable/disable back-face culling");

    if (faceCulling)
    {
        ImGui::Indent();
        int cullMode = renderer->GetCullFaceMode();
        const char* cullModes[] = { "Back", "Front", "Front and Back" };
        if (ImGui::Combo("Cull Face Mode", &cullMode, cullModes, 3))
        {
            renderer->SetCullFaceMode(cullMode);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Which faces to cull:\n- Back: Normal (hides back faces)\n- Front: Rare (hides front faces)\n- Both faces");
        ImGui::Unindent();
    }

    ImGui::Spacing();

    bool wireframe = renderer->IsWireframeMode();
    if (ImGui::Checkbox("Wireframe Mode", &wireframe))
    {
        renderer->SetWireframeMode(wireframe);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Render meshes as wireframes");

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("Background Color");

    float currentR, currentG, currentB;
    renderer->GetClearColor(currentR, currentG, currentB);

    static float editColor[3] = { currentR, currentG, currentB };

    ImGui::ColorEdit3("##ClearColor", editColor, ImGuiColorEditFlags_NoAlpha);

    bool colorChanged = (editColor[0] != currentR || editColor[1] != currentG || editColor[2] != currentB);

    ImGui::SameLine();

    if (!colorChanged)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Apply"))
    {
        renderer->SetClearColor(editColor[0], editColor[1], editColor[2]);
        LOG_CONSOLE("Background color updated");
        LOG_DEBUG("Background color set to (%.2f, %.2f, %.2f)", editColor[0], editColor[1], editColor[2]);
    }

    if (!colorChanged)
    {
        ImGui::EndDisabled();
    }

    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Apply the new background color");

    ImGui::SameLine();

    if (ImGui::Button("Reset Color"))
    {
        editColor[0] = 0.2f;
        editColor[1] = 0.25f;
        editColor[2] = 0.3f;
        renderer->SetClearColor(editColor[0], editColor[1], editColor[2]);
        LOG_CONSOLE("Background color reset to default");
        LOG_DEBUG("Background color reset to default");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset background color to default");

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("Reset All Settings"))
    {
        renderer->SetFaceCulling(true);
        renderer->SetWireframeMode(false);
        renderer->SetCullFaceMode(0);
        renderer->SetClearColor(0.2f, 0.25f, 0.3f);

        editColor[0] = 0.2f;
        editColor[1] = 0.25f;
        editColor[2] = 0.3f;

        LOG_CONSOLE("All renderer settings reset to defaults");
        LOG_DEBUG("All renderer settings reset to defaults");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Debug Visualization");

    if (ImGui::Checkbox("Show AABB", &showAABB))
    {
        LOG_DEBUG("AABB visualization: %s", showAABB ? "ON" : "OFF");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show axis-aligned bounding boxes for all meshes");

    if (ImGui::Checkbox("Show Octree", &showOctree))
    {
        LOG_DEBUG("Octree visualization: %s", showOctree ? "ON" : "OFF");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show octree spatial partitioning structure");

    if (ImGui::Checkbox("Show Raycast", &showRaycast))
    {
        LOG_DEBUG("Raycast visualization: %s", showRaycast ? "ON" : "OFF");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show ray used for mouse picking");

    ImGui::Spacing();
}

void ConfigurationWindow::DrawPhysicsSettings()
{
    ModulePhysics* physics = Application::GetInstance().physics.get();
    if (!physics)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Physics module not available");
        return;
    }

    ImGui::Text("Physics Engine: Bullet Physics");
    ImGui::Separator();

    // Gravity configuration
    ImGui::Text("Gravity");
    glm::vec3 gravity = physics->GetGravity();

    if (ImGui::DragFloat3("##gravity", &gravity.x, 0.1f, -50.0f, 50.0f, "%.2f"))
    {
        physics->SetGravity(gravity);
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Gravity acceleration (m/s²)\nEarth gravity: (0, -9.81, 0)");
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset##gravity"))
    {
        physics->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Simulation settings
    ImGui::Text("Simulation Settings");

    int substeps = physics->GetSubsteps();
    if (ImGui::SliderInt("Max Substeps", &substeps, 1, 20))
    {
        physics->SetSubsteps(substeps);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("More substeps = more accurate but slower\nRecommended: 10");
    }

    float fixedTimeStep = physics->GetFixedTimeStep();
    if (ImGui::SliderFloat("Fixed Timestep", &fixedTimeStep, 0.001f, 0.1f, "%.4f"))
    {
        physics->SetFixedTimeStep(fixedTimeStep);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Internal physics timestep\nDefault: 1/60 = 0.0167");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Debug visualization
    ImGui::Text("Debug Visualization");

    bool debugDraw = physics->IsDebugDrawEnabled();
    if (ImGui::Checkbox("Show Physics Debug", &debugDraw))
    {
        physics->SetDebugDrawEnabled(debugDraw);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Quick presets
    ImGui::Text("Quick Presets");

    if (ImGui::Button("Earth Gravity"))
    {
        physics->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    }
    ImGui::SameLine();
    if (ImGui::Button("Moon Gravity"))
    {
        physics->SetGravity(glm::vec3(0.0f, -1.62f, 0.0f));
    }
    ImGui::SameLine();
    if (ImGui::Button("Zero Gravity"))
    {
        physics->SetGravity(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Information
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Info:");
    ImGui::TextWrapped("Physics simulation is linked to play mode. Use Play/Pause/Stop controls to manage simulation.");
}