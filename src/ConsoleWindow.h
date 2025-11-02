#pragma once
#include "EditorWindow.h"
#include <vector>
#include <string>

enum class LogType
{
    INFO,
    WARNING,
    ERROR_LOG
};

struct LogEntry
{
    std::string message;
    LogType type;
};

class ConsoleWindow : public EditorWindow
{
public:
    ConsoleWindow();
    ~ConsoleWindow();

    void Draw() override;

    static void AddLog(const std::string& message, LogType type = LogType::INFO);
    static void Clear();

private:
    static std::vector<LogEntry> logs;
    bool autoScroll;
    bool showInfo;
    bool showWarnings;
    bool showErrors;
};