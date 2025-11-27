#pragma once
#include "Module.h"
#include <vector>
#include <string>
#include <deque>
#include <imgui.h>
#include <ImGuizmo.h>
#include "PrimitiveGenerator.h"
#include <glm/gtc/quaternion.hpp>

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

    void MarkForDeletion(GameObject* go);
    void ProcessDeletions();

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

    // Selected GameObjects 
    std::vector<GameObject*> selectedGameObjects;

    // Helper para verificar si un objeto está seleccionado
    bool IsSelected(GameObject* go) const;

    // Métodos para manejo de selección
    void SelectGameObject(GameObject* go, bool includeDescendants = true);
    void DeselectAll();
    glm::vec3 GetSelectionCenter() const;
    int CountNames(std::string prefix);

    bool isMouseOverTextureDropZone = false;
    ImVec2 textureDropPos;
    ImVec2 textureDropSize;

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
    void SafeDeleteGameObject(GameObject* go);

    std::vector<GameObject*> m_ObjectsToDelete;

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
        // Menu Bar
        float menuBarHeight = 25.0f;

        // Scene
        float sceneXPercent = 0.15f;
        float sceneWidthPercent = 0.70f;
        float sceneHeightPercent = 0.80f;

        // Hierarchy
        float hierarchyXPercent = 0.0f;
        float hierarchyYPercent = 0.5f;
        float hierarchyWidthPercent = 0.15f;
        float hierarchyHeightPercent = 0.8f;

        // Inspector
        float inspectorXPercent = 0.85f;
        float inspectorWidthPercent = 0.15f;
        float inspectorHeightPercent = 1.0f;

        // Console
        float consoleYPercent = 0.80f;
        float consoleXPercent = 0.f;
        float consoleWidthPercent = 0.85f;
        float consoleHeightPercent = 0.20f;

        // Margins
        float marginX = 5.0f;
        float marginY = 5.0f;
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

    // Hierarchy drag & drop
    GameObject* draggedGameObject = nullptr;
    bool isDragging = false;

    // Helper functions for hierarchy
    void DrawGameObjectNode(GameObject* go);
    void HandleHierarchyDragDrop(GameObject* go);

    // Confirmation dialogs
    bool showNewSceneConfirmation = false;
    bool showLoadSceneConfirmation = false;
    std::string pendingSceneToLoad = "";

    glm::quat lastMultiSelectionRotation = glm::quat(1, 0, 0, 0);
    glm::vec3 lastMultiSelectionScale = glm::vec3(1, 1, 1);
    bool wasManipulating = false;
};

// Global logging functions
extern ModuleEditor* g_Editor;
void LOG(const std::string& message);
void LOG_WARNING(const std::string& message);
void LOG_ERROR(const std::string& message);