#pragma once
#include "Module.h"
#include "imgui.h"
#include "imguizmo.h"
#include "EditorCommand.h"
#include <string>
#include <deque>
#include <vector>
#include <memory>
#include <glm/gtc/quaternion.hpp>
#include <map>

// Forward declarations
class EditorWindow;
class SceneViewportWindow;
class HierarchyWindow;
class InspectorWindow;
class ConsoleWindow;
class ConfigurationWindow;
class AboutWindow;
class MenuBarWindow;
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

struct EditorLayout
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

void LOG(const std::string& message);
void LOG_WARNING(const std::string& message);
void LOG_ERROR(const std::string& message);

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

    void AddLog(const std::string& message, LogType type);
    void ClearLog();

    void UpdateLayout(int windowWidth, int windowHeight);
    void ResetLayout();

    static void SetupImGuiStyle();

    void AssignCheckerboardTexture(GameObject* go);
    int CountNames(std::string prefix);

    // Selection management
    bool IsSelected(GameObject* go) const;
    void SelectGameObject(GameObject* go, bool includeDescendants);
    void DeselectAll();
    glm::vec3 GetSelectionCenter() const;

    // Deletion management
    void MarkForDeletion(GameObject* go);
    void ProcessDeletions();

    // Scene management
    void RefreshScenesList();
    void SaveSceneDialog();
    void LoadSceneDialog();
    bool SaveScene(const std::string& filepath);
    bool LoadScene(const std::string& filepath);
    void ClearCurrentScene();

    void DrawGuizmo();

public:
    // Windows
    std::unique_ptr<SceneViewportWindow> sceneViewportWindow;
    std::unique_ptr<HierarchyWindow> hierarchyWindow;
    std::unique_ptr<InspectorWindow> inspectorWindow;
    std::unique_ptr<ConsoleWindow> consoleWindow;
    std::unique_ptr<ConfigurationWindow> configurationWindow;
    std::unique_ptr<AboutWindow> aboutWindow;
    std::unique_ptr<MenuBarWindow> menuBarWindow;
	std::unique_ptr<EditorWindow> assetsWindow;

    // Window visibility flags
    bool showConsole = false;
    bool showConfiguration = false;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAbout = false;
	bool showAssets = true;

    // Layout
    EditorLayout layout;
    bool firstTimeSetup = true;
    bool useAdaptiveLayout = false;
    int lastWindowWidth = 0;
    int lastWindowHeight = 0;

    // Scene viewport
    ImVec2 sceneViewportPos;
    ImVec2 sceneViewportSize;

    // Texture drop area
    ImVec2 textureDropPos;
    ImVec2 textureDropSize;

    // Console
    std::deque<LogEntry> logs;
    const size_t maxLogs = 1000;
    bool autoScroll = true;

    // FPS
    std::vector<float> fpsHistory;
    const size_t maxFPSHistory = 100;
    float lastFrameTime = 0.0f;

    // Configuration window flags
    bool showAllVertexNormals = false;
    bool showAllFaceNormals = false;
    bool showAllAABBs = false;

    // Selection
    std::vector<GameObject*> selectedGameObjects;

    // Gizmo
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
    bool editing = false;
    bool sceneEditing = false;
    bool updatedAngles = false;

    // Multi-selection gizmo tracking
    bool wasManipulating = false;
    glm::quat lastMultiSelectionRotation = glm::quat(1, 0, 0, 0);
    glm::vec3 lastMultiSelectionScale = glm::vec3(1, 1, 1);
    glm::vec3 initialMultiSelectionScale = glm::vec3(1, 1, 1);

    // Scene management
    std::string currentScenePath = "";
    bool sceneModified = false;
    std::vector<std::string> availableScenes;

    // Scene dialogs
    bool showNewSceneConfirmation = false;
    bool showSaveDialog = false;
    bool showLoadDialog = false;
    bool showLoadSceneConfirmation = false;
    std::string pendingSceneToLoad = "";
    char saveSceneNameBuffer[128] = "";

    // About info
    const char* motorName = "Ilium Engine";
    const char* version = "v0.9";
    const char* team = "Team Hutao";
    const char* repoURL = "https://github.com/Bekun67/Motor";

    // Command history for undo/redo
    CommandHistory commandHistory;

    // Capture transform state before manipulation
    void BeginTransformEdit(GameObject* go);
    void EndTransformEdit(GameObject* go);

    // File dialogs using native system
    std::string OpenFileDialog(const char* filter);
    std::string SaveFileDialog(const char* filter);

    // Copy/Paste/Duplicate
    void CopySelectedObjects();
    void PasteObjects();
    void DuplicateSelectedObjects();
    bool HasCopiedObjects() const { return !m_CopiedObjects.empty(); }

private:
    std::vector<GameObject*> m_ObjectsToDelete;

    // Store transform state for undo/redo
    struct TransformState
    {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
    };
    std::map<GameObject*, TransformState> m_TransformStates;

    // Clipboard for copy/paste
    struct CopiedObjectData
    {
        std::string name;
        std::string meshPath;
        int meshIndexInFBX;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        std::string texturePath;
        GameObject* originalParent;
    };
    std::vector<CopiedObjectData> m_CopiedObjects;

    // Multi-object transform tracking
    struct MultiTransformState
    {
        std::vector<GameObject*> objects;
        std::vector<glm::vec3> positions;
        std::vector<glm::quat> rotations;
        std::vector<glm::vec3> scales;
    };
    MultiTransformState m_MultiTransformState;
    bool m_TrackingMultiTransform = false;
};

extern ModuleEditor* g_Editor;