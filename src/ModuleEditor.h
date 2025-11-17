#pragma once
#include "Module.h"
#include <vector>
#include <string>
#include <deque>
#include <imgui.h>
#include <ImGuizmo.h>
#include "PrimitiveGenerator.h"

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

    // Scene Serialization
    void SaveSceneDialog();
    void LoadSceneDialog();
    bool SaveScene(const std::string& filepath);
    bool LoadScene(const std::string& filepath);

    static void SetupImGuiStyle();

    // Layout management
    void UpdateLayout(int windowWidth, int windowHeight);
    void ResetLayout();

    // ImGuizmo
    void DrawGuizmo();
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

    // Window visibility toggles
    bool showConsole = true;
    bool showConfiguration = false;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAbout = false;

    bool editing = false;
    bool updatedAngles = false;

    ImVec2 sceneViewportPos;
    ImVec2 sceneViewportSize;

    // Selected GameObject
    GameObject* selectedGameObject = nullptr;
    int CountNames(std::string prefix);

    bool isMouseOverTextureDropZone = false;
    ImVec2 textureDropZoneMin;
    ImVec2 textureDropZoneMax;

    bool showAllAABBs = false;
    bool showAllVertexNormals = false;
    bool showAllFaceNormals = false;

    // Scene management
    std::string currentScenePath = "";
    bool sceneModified = false;

private:
    void DrawMenuBar();
    void DrawConsole();
    void DrawConfiguration();
    void DrawHierarchy();
    void DrawInspector();
    void DrawAbout();
    void AssignCheckerboardTexture(GameObject* go);
    void DrawSceneViewport();

    // Console
    std::deque<LogEntry> logs;
    const size_t maxLogs = 1000;
    bool autoScroll = true;

    // Configuration - FPS tracking
    std::vector<float> fpsHistory;
    const size_t maxFPSHistory = 100;
    float lastFrameTime = 0.0f;

    // About window info
    const char* motorName = "Ilium Engine";
    const char* version = "v0.5.0";
    const char* team = "Team Hutao";
    const char* repoURL = "https://github.com/Bekun67/Motor";

    bool firstTimeSetup = true;
    bool useAdaptiveLayout = true;

    // Layout percentages (relative to window size)
    struct LayoutConfig
    {
        // Menu
        float menuWidthPercent = 0.12f;  // 12% of width
        float menuHeightPercent = 0.44f; // 44% of height

        // Scene
        float sceneXPercent = 0.13f;      // Starting at 13% from left
        float sceneWidthPercent = 0.64f;  // 64% of width
        float sceneHeightPercent = 0.76f; // 76% of height

        // Hierarchy
        float hierarchyXPercent = 0.0f;
        float hierarchyYPercent = 0.48f;
        float hierarchyWidthPercent = 0.12f;
        float hierarchyHeightPercent = 0.52f;

        // Inspector
        float inspectorXPercent = 0.78f;
        float inspectorWidthPercent = 0.22f;
        float inspectorHeightPercent = 1.0f;

        // Console
        float consoleYPercent = 0.78f;
        float consoleXPercent = 0.13f;
        float consoleWidthPercent = 0.64f;
        float consoleHeightPercent = 0.22f;

        // Margins
        float marginX = 10.0f;
        float marginY = 20.0f;
    };

    LayoutConfig layout;
    int lastWindowWidth = 0;
    int lastWindowHeight = 0;

    // Save/Load dialog state
    char saveSceneNameBuffer[256] = "NewScene";
    bool showSaveDialog = false;
    bool showLoadDialog = false;
    std::vector<std::string> availableScenes;
    void RefreshScenesList();
};

// Global logging functions
extern ModuleEditor* g_Editor;
void LOG(const std::string& message);
void LOG_WARNING(const std::string& message);
void LOG_ERROR(const std::string& message);