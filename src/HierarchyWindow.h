#pragma once
#include "EditorWindow.h"
#include "imgui.h"

class GameObject;

class HierarchyWindow : public EditorWindow
{
public:
    HierarchyWindow(ModuleEditor* editor);
    ~HierarchyWindow() override = default;

    void Draw() override;

private:
    void DrawGameObjectNode(GameObject* go);
    void HandleHierarchyDragDrop(GameObject* target);
};