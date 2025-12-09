#include "SceneViewportWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"

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
    ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    editor->sceneViewportPos = viewportPos;
    editor->sceneViewportSize = viewportSize;

    ImGui::End();
    ImGui::PopStyleVar();
}