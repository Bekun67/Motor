#pragma once
#include "EditorWindow.h"
#include "imgui.h"

class InspectorWindow : public EditorWindow
{
public:
    InspectorWindow(ModuleEditor* editor);
    ~InspectorWindow() override = default;

    void Draw() override;

private:
    void DrawSingleObjectInspector();
    void DrawMultiObjectInspector();
};