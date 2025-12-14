#pragma once
#include <string>

class ModuleEditor;

class EditorWindow
{
public:
    EditorWindow(ModuleEditor* editor, const std::string& name);
    virtual ~EditorWindow() = default;

    virtual void Draw() = 0;

    bool IsVisible() const { return visible; }
    void SetVisible(bool visible) { this->visible = visible; }
    const std::string& GetName() const { return windowName; }

protected:
    ModuleEditor* editor;
    std::string windowName;
    bool visible;
};