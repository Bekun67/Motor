#include "EditorWindow.h"
#include "ModuleEditor.h"

EditorWindow::EditorWindow(ModuleEditor* editor, const std::string& name)
    : editor(editor), windowName(name), visible(true)
{
}