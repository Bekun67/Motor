#pragma once

#include <SDL3/SDL.h>
#include "Module.h"

class Window : public Module
{
public:
    Window();
    ~Window();

    bool Start() override;

    // Handle events (returns false if quit requested)
    bool Update() override;

    // Clear screen and present
    void Render();

    bool PostUpdate() override;

    // Clean up resources
    bool CleanUp() override;

    // Getters
    void GetWindowSize(int& width, int& height) const;
    int GetScale() const;

    SDL_Window* GetWindow() const { return window; }

    // Check if window was resized this frame
    bool WasResized() const { return wasResized; }
    void ResetResizeFlag() { wasResized = false; }

private:
    SDL_Window* window;

    int width;
    int height;
    int scale;
    bool wasResized = false;
};