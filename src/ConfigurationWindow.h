#pragma once
#include "EditorWindow.h"
#include "imgui.h"

class ConfigurationWindow : public EditorWindow
{
public:
    ConfigurationWindow(ModuleEditor* editor);
    ~ConfigurationWindow() override = default;

    void Draw() override;

	bool showOctreeStats = false;
	bool showCullingStats = false;
};