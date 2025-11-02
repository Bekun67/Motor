#pragma once
#include "EditorWindow.h"

class GameObject;

class HierarchyWindow : public EditorWindow
{
public:
    HierarchyWindow();
    ~HierarchyWindow();

    void Draw() override;

    static GameObject* GetSelectedGameObject() { return selectedGameObject; }
    static void SetSelectedGameObject(GameObject* go) { selectedGameObject = go; }

private:
    void DrawGameObjectNode(GameObject* go);

    static GameObject* selectedGameObject;
};