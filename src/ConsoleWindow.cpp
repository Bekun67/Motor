#include "ConsoleWindow.h"
#include <imgui.h>

std::vector<LogEntry> ConsoleWindow::logs;

ConsoleWindow::ConsoleWindow()
    : EditorWindow("Console", true),
    autoScroll(true),
    showInfo(true),
    showWarnings(true),
    showErrors(true)
{
}

ConsoleWindow::~ConsoleWindow()
{
}

void ConsoleWindow::Draw()
{
    if (!ImGui::Begin(name.c_str(), &visible))
    {
        ImGui::End();
        return;
    }

    // Filter options
    ImGui::Checkbox("Info", &showInfo); ImGui::SameLine();
    ImGui::Checkbox("Warnings", &showWarnings); ImGui::SameLine();
    ImGui::Checkbox("Errors", &showErrors); ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll); ImGui::SameLine();

    if (ImGui::Button("Clear"))
    {
        Clear();
    }

    ImGui::Separator();

    // Log display
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const LogEntry& log : logs)
    {
        bool shouldShow = false;
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        switch (log.type)
        {
        case LogType::INFO:
            shouldShow = showInfo;
            color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            break;
        case LogType::WARNING:
            shouldShow = showWarnings;
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            break;
        case LogType::ERROR_LOG:
            shouldShow = showErrors;
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            break;
        }

        if (shouldShow)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(log.message.c_str());
            ImGui::PopStyleColor();
        }
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

void ConsoleWindow::AddLog(const std::string& message, LogType type)
{
    LogEntry entry;
    entry.message = message;
    entry.type = type;
    logs.push_back(entry);

    // Keep only last 1000 entries
    if (logs.size() > 1000)
    {
        logs.erase(logs.begin());
    }
}

void ConsoleWindow::Clear()
{
    logs.clear();
}