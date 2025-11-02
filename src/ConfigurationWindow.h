#pragma once
#include "EditorWindow.h"
#include <vector>
#include <deque>

class ConfigurationWindow : public EditorWindow
{
public:
    ConfigurationWindow();
    ~ConfigurationWindow();

    void Draw() override;
    void AddFPS(float fps);

private:
    void DrawApplicationSettings();
    void DrawWindowSettings();
    void DrawRendererSettings();
    void DrawCameraSettings();
    void DrawHardwareInfo();
    void DrawFPSGraph();

    std::deque<float> fpsHistory;
    static const int MAX_FPS_HISTORY = 100;
};