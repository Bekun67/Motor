#pragma once
#include "EditorWindow.h"
#include "imgui.h"

class AboutWindow : public EditorWindow
{
public:
    AboutWindow(ModuleEditor* editor);
    ~AboutWindow() override = default;

    void Draw() override;
};