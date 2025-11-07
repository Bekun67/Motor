#pragma once
#include "Module.h"
#include <vector>
#include <string>
#include <deque>

class GameObject;

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

    LogEntry(const std::string& msg, LogType t) : message(msg), type(t) {}
};

class ModuleEditor : public Module
{
public:
    ModuleEditor();
    ~ModuleEditor();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    // Log system
    void AddLog(const std::string& message, LogType type = LogType::INFO);
    void ClearLog();

    // Window visibility toggles
    bool showConsole = true;
    bool showConfiguration = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAbout = false;

    bool editing = false;
    bool updatedAngles = false;

private:
    void DrawMenuBar();
    void DrawConsole();
    void DrawConfiguration();
    void DrawHierarchy();
    void DrawInspector();
    void DrawAbout();
    void AssignCheckerboardTexture(GameObject* go);

    // Console
    std::deque<LogEntry> logs;
    const size_t maxLogs = 1000;
    bool autoScroll = true;

    // Configuration - FPS tracking
    std::vector<float> fpsHistory;
    const size_t maxFPSHistory = 100;
    float lastFrameTime = 0.0f;

    // Selected GameObject
    GameObject* selectedGameObject = nullptr;

    // About window info
    const char* motorName = "Ilium Engine";
    const char* version = "v0.1.5";
    const char* team = "Team Hutao";
    const char* repoURL = "https://github.com/Bekun67/Motor";
};

// Global logging functions
extern ModuleEditor* g_Editor;
void LOG(const std::string& message);
void LOG_WARNING(const std::string& message);
void LOG_ERROR(const std::string& message);