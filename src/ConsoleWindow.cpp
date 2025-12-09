#include "ConsoleWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"

ConsoleWindow::ConsoleWindow(ModuleEditor* editor)
    : EditorWindow(editor, "Console")
{
}

void ConsoleWindow::Draw()
{
    if (!visible) return;

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (editor->firstTimeSetup)
    {
        float x = windowWidth * editor->layout.consoleXPercent + editor->layout.marginX;
        float y = windowHeight * editor->layout.consoleYPercent;
        float width = windowWidth * editor->layout.consoleWidthPercent;
        float height = windowHeight * editor->layout.consoleHeightPercent - editor->layout.marginY;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (editor->useAdaptiveLayout)
    {
        float x = windowWidth * editor->layout.consoleXPercent;
        float y = windowHeight * editor->layout.consoleYPercent;
        float width = windowWidth * editor->layout.consoleWidthPercent;
        float height = windowHeight * editor->layout.consoleHeightPercent - editor->layout.marginY;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Console", &visible);

    if (ImGui::Button("Clear"))
        editor->ClearLog();

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &editor->autoScroll);

    ImGui::Separator();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& log : editor->logs)
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

    // See auto the last Log
    if (editor->autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}