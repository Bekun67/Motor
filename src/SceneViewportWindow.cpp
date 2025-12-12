#include "SceneViewportWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"
#include "EditorPlaySystem.h"

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

    ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Draw Play/Pause/Stop controls in the same line as title
    DrawPlayControls();

    ImGui::Separator();

    // Now get viewport area
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    editor->sceneViewportPos = viewportPos;
    editor->sceneViewportSize = viewportSize;

    ImGui::PopStyleVar();

    ImGui::End();
}

void SceneViewportWindow::DrawPlayControls()
{
    bool isPlaying = EditorPlaySystem::IsPlaying();
    bool isPaused = EditorPlaySystem::IsPaused();
    bool isStopped = EditorPlaySystem::IsStopped();

    // Move to same line as window title
    ImGui::SameLine();

    // Push to the right
    float windowWidth = ImGui::GetWindowWidth();
    float buttonWidth = 60.0f;
    float padding = 5.0f;
    float totalWidth = (buttonWidth * 3) + (padding * 2);
    float offsetX = windowWidth - totalWidth - 40.0f; // 40px from right edge for close button

    ImGui::SameLine(offsetX);

    ImVec2 buttonSize(buttonWidth, 20.0f); // Smaller height for title bar

    // PLAY button
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));

    if (isPlaying && !isPaused)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.4f, 0.15f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));

    if (ImGui::Button("Play", buttonSize))
    {
        if (isStopped || isPaused)
        {
            EditorPlaySystem::Play();
        }
    }
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Play (F5)");

    ImGui::SameLine();

    // PAUSE button
    ImGui::BeginDisabled(isStopped);

    if (isPaused)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.5f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.3f, 0.15f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.6f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.4f, 0.1f, 1.0f));

    if (ImGui::Button("Pause", buttonSize))
    {
        if (isPlaying)
        {
            EditorPlaySystem::Pause();
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    if (ImGui::IsItemHovered() && !isStopped)
        ImGui::SetTooltip("Pause (F6)");

    ImGui::SameLine();

    // STOP button
    ImGui::BeginDisabled(isStopped);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Stop", buttonSize))
    {
        EditorPlaySystem::Stop();
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    ImGui::PopStyleVar(2); // FrameRounding and FramePadding

    if (ImGui::IsItemHovered() && !isStopped)
        ImGui::SetTooltip("Stop (F7)");
}