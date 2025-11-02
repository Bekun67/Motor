#pragma once
#include "Module.h"
#include <vector>
#include <string>

class EditorWindow;

class ImGuiModule : public Module
{
public:
    ImGuiModule();
    ~ImGuiModule();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    // Window management
    void AddEditorWindow(EditorWindow* window);
    void ShowMainMenuBar();

    std::vector<EditorWindow*>& GetEditorWindows() { return editorWindows; }

private:
    std::vector<EditorWindow*> editorWindows;

    // Menu options
    void ShowFileMenu();
    void ShowViewMenu();
    void ShowHelpMenu();
    void ShowAboutWindow();
    void ShowGeometryMenu();

    bool showAboutWindow;
};