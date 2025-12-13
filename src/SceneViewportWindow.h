#pragma once
#include "EditorWindow.h"
#include "imgui.h"

class SceneViewportWindow : public EditorWindow
{
public:
    SceneViewportWindow(ModuleEditor* editor);
    ~SceneViewportWindow() override = default;

    void Draw() override;

private:
    void DrawPlayControls();

    void HandleMeshDrop(const std::string& meshPath, float mouseX, float mouseY);
    void HandleTextureDrop(const std::string& texturePath);
};