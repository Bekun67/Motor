#include "AboutWindow.h"
#include "ModuleEditor.h"
#include <SDL3/SDL.h>

AboutWindow::AboutWindow(ModuleEditor* editor)
    : EditorWindow(editor, "About")
{
}

void AboutWindow::Draw()
{
    if (!visible) return;

    if (editor->firstTimeSetup)
    {
        ImGui::SetNextWindowPos(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_Always);
    }

    ImGui::Begin("About", &visible);

    ImGui::Text("%s", editor->motorName);
    ImGui::Text("Version: %s", editor->version);
    ImGui::Separator();

    ImGui::Text("Team:");
    ImGui::BulletText("%s", editor->team);
    ImGui::Separator();

    ImGui::Text("Libraries Used:");
    ImGui::BulletText("SDL3");
    ImGui::BulletText("OpenGL");
    ImGui::BulletText("GLAD");
    ImGui::BulletText("ImGui");
    ImGui::BulletText("ImGuizmo");
    ImGui::BulletText("DevIL");
    ImGui::BulletText("Assimp");
    ImGui::BulletText("GLM");
    ImGui::BulletText("Nlohmann Json");
    ImGui::Separator();

    ImGui::Text("License: MIT License");
    ImGui::Separator();

    if (ImGui::Button("GitHub Repository"))
    {
        SDL_OpenURL(editor->repoURL);
    }

    ImGui::End();
}