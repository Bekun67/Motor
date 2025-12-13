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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    editor->sceneViewportPos = viewportPos;
    editor->sceneViewportSize = viewportSize;

    ImGui::End();
    ImGui::PopStyleVar();

    DrawPlayControls();
}

void SceneViewportWindow::DrawPlayControls()
{
    bool isPlaying = EditorPlaySystem::IsPlaying();
    bool isPaused = EditorPlaySystem::IsPaused();
    bool isStopped = EditorPlaySystem::IsStopped();

    // Position, on top of the Scene window
    float controlsX = editor->sceneViewportPos.x;
    float controlsY = editor->sceneViewportPos.y - 30.0f; 

    // buttons size
    ImVec2 buttonSize(80, 28);
    float totalWidth = buttonSize.x * 3 + ImGui::GetStyle().ItemSpacing.x * 2;

    controlsX += (editor->sceneViewportSize.x - totalWidth) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(controlsX, controlsY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(totalWidth + 20, buttonSize.y + 4));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 2));
    ImGui::Begin("##PlayControls", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBackground);

    // PLAY button
    if (isPlaying && !isPaused)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));

    if (ImGui::Button("Play", buttonSize))
    {
        if (isStopped || isPaused)
        {
            EditorPlaySystem::Play();
        }
    }
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Start game simulation (F5)");

    ImGui::SameLine();

    // PAUSE button
    ImGui::BeginDisabled(isStopped);

    if (isPaused)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.4f, 0.2f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.5f, 0.1f, 1.0f));

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
        ImGui::SetTooltip("Pause game simulation (F6)");

    ImGui::SameLine();

    // STOP button
    ImGui::BeginDisabled(isStopped);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Stop", buttonSize))
    {
        EditorPlaySystem::Stop();
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    if (ImGui::IsItemHovered() && !isStopped)
        ImGui::SetTooltip("Stop game and restore scene (F7)");

    ImGui::End();
    ImGui::PopStyleVar();
}