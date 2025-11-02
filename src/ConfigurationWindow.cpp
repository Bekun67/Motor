#include "ConfigurationWindow.h"
#include "Application.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>

ConfigurationWindow::ConfigurationWindow()
    : EditorWindow("Configuration", true)
{
}

ConfigurationWindow::~ConfigurationWindow()
{
}

void ConfigurationWindow::Draw()
{
    if (!ImGui::Begin(name.c_str(), &visible))
    {
        ImGui::End();
        return;
    }

    DrawFPSGraph();
    ImGui::Separator();
    DrawApplicationSettings();
    ImGui::Separator();
    DrawWindowSettings();
    ImGui::Separator();
    DrawRendererSettings();
    ImGui::Separator();
    DrawCameraSettings();
    ImGui::Separator();
    DrawHardwareInfo();

    ImGui::End();
}

void ConfigurationWindow::AddFPS(float fps)
{
    fpsHistory.push_back(fps);
    if (fpsHistory.size() > MAX_FPS_HISTORY)
    {
        fpsHistory.pop_front();
    }
}

void ConfigurationWindow::DrawFPSGraph()
{
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (!fpsHistory.empty())
        {
            float fps = fpsHistory.back();
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Frame Time: %.3f ms", 1000.0f / fps);

            // Convert deque to vector for plotting
            std::vector<float> fpsVector(fpsHistory.begin(), fpsHistory.end());

            ImGui::PlotHistogram("##FPS", fpsVector.data(), (int)fpsVector.size(),
                0, nullptr, 0.0f, 120.0f, ImVec2(0, 80));
        }
    }
}

void ConfigurationWindow::DrawApplicationSettings()
{
    if (ImGui::CollapsingHeader("Application"))
    {
        static char appName[64] = "Game Engine Motor";
        ImGui::InputText("App Name", appName, 64);

        static char organization[64] = "Your Organization";
        ImGui::InputText("Organization", organization, 64);

        ImGui::Text("Max FPS: ");
        ImGui::SameLine();
        static int maxFPS = 60;
        if (ImGui::SliderInt("##MaxFPS", &maxFPS, 1, 144))
        {
            // TODO: Apply max FPS limit
        }
    }
}

void ConfigurationWindow::DrawWindowSettings()
{
    if (ImGui::CollapsingHeader("Window"))
    {
        int width, height;
        Application::GetInstance().window->GetWindowSize(width, height);

        ImGui::Text("Width: %d", width);
        ImGui::Text("Height: %d", height);

        static bool fullscreen = false;
        if (ImGui::Checkbox("Fullscreen", &fullscreen))
        {
            // TODO: Toggle fullscreen
        }

        static bool resizable = true;
        if (ImGui::Checkbox("Resizable", &resizable))
        {
            // TODO: Toggle resizable
        }

        static bool borderless = false;
        if (ImGui::Checkbox("Borderless", &borderless))
        {
            // TODO: Toggle borderless
        }

        static float brightness = 1.0f;
        if (ImGui::SliderFloat("Brightness", &brightness, 0.0f, 1.0f))
        {
            SDL_Window* window = Application::GetInstance().window->GetWindow();
            // Note: SDL3 might handle this differently
        }
    }
}

void ConfigurationWindow::DrawRendererSettings()
{
    if (ImGui::CollapsingHeader("Renderer"))
    {
        bool showGrid = Application::GetInstance().opengl->showGrid;
        if (ImGui::Checkbox("Show Grid", &showGrid))
        {
            Application::GetInstance().opengl->showGrid = showGrid;
        }

        static bool vsync = true;
        if (ImGui::Checkbox("VSync", &vsync))
        {
            SDL_GL_SetSwapInterval(vsync ? 1 : 0);
        }

        static bool depthTest = true;
        if (ImGui::Checkbox("Depth Test", &depthTest))
        {
            if (depthTest)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
        }

        static bool cullFace = false;
        if (ImGui::Checkbox("Face Culling", &cullFace))
        {
            if (cullFace)
                glEnable(GL_CULL_FACE);
            else
                glDisable(GL_CULL_FACE);
        }

        static bool wireframe = false;
        if (ImGui::Checkbox("Wireframe", &wireframe))
        {
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }
    }
}

void ConfigurationWindow::DrawCameraSettings()
{
    if (ImGui::CollapsingHeader("Camera"))
    {
        // TODO: Access camera from OpenGL module
        ImGui::Text("Camera settings will be implemented");
        ImGui::Text("when camera is accessible");
    }
}

void ConfigurationWindow::DrawHardwareInfo()
{
    if (ImGui::CollapsingHeader("Hardware & Software Info"))
    {
        ImGui::Text("SDL Version:");
        ImGui::BulletText("%d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

        ImGui::Text("OpenGL Version:");
        ImGui::BulletText("%s", glGetString(GL_VERSION));

        ImGui::Text("GLSL Version:");
        ImGui::BulletText("%s", glGetString(GL_SHADING_LANGUAGE_VERSION));

        ImGui::Text("Vendor:");
        ImGui::BulletText("%s", glGetString(GL_VENDOR));

        ImGui::Text("Renderer:");
        ImGui::BulletText("%s", glGetString(GL_RENDERER));

        // Memory info (platform specific)
        ImGui::Text("System Memory:");
        ImGui::BulletText("%d MB RAM", SDL_GetSystemRAM());
    }
}