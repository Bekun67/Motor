#pragma once
#include "EditorWindow.h"
#include "imgui.h"

class ConsoleWindow : public EditorWindow
{
public:
    ConsoleWindow(ModuleEditor* editor);
    ~ConsoleWindow() override = default;

    void Draw() override;
};