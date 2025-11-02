#pragma once
#include "EditorWindow.h"

class GameObject;

class InspectorWindow : public EditorWindow
{
public:
    InspectorWindow();
    ~InspectorWindow();

    void Draw() override;

private:
    void DrawTransformComponent(GameObject* go);
    void DrawMeshComponent(GameObject* go);
    void DrawTextureComponent(GameObject* go);

    bool showNormals;
    bool showCheckerTexture;
};