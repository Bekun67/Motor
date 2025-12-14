#pragma once
#include "EditorWindow.h"
#include "imgui.h"

class MenuBarWindow : public EditorWindow
{
public:
    MenuBarWindow(ModuleEditor* editor);
    ~MenuBarWindow() override = default;

    void Draw() override;

private:
    void DrawFileMenu();
    void DrawEditMenu();
    void DrawViewMenu();
    void DrawGameObjectMenu();
    void DrawHelpMenu();
    void DrawPlayControls();
    void DrawPopups();
};