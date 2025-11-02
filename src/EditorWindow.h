#pragma once
#include <string>

class EditorWindow
{
public:
    EditorWindow(const std::string& name, bool visible = true);
    virtual ~EditorWindow();

    virtual void Draw() = 0;

    bool IsVisible() const { return visible; }
    void SetVisible(bool visible) { this->visible = visible; }
    std::string GetName() const { return name; }

protected:
    std::string name;
    bool visible;
};