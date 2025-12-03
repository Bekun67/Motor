#include "Window.h"
#include <iostream>

Window::Window() : window(nullptr), width(1280), height(720), scale(1), wasResized(false)
{
    std::cout << "Window Constructor" << std::endl;
}

Window::~Window()
{
}

bool Window::Start()
{
    std::cout << "Init SDL3 Window" << std::endl;

    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set OpenGL version to 3.3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Use the core OpenGL profile (modern functions only)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Enable double buffering to prevent flickering
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Set depth buffer to 24 bits for proper 3D rendering
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Enable stencil buffer
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window WITH OpenGL flag
    window = SDL_CreateWindow(
        "Ilium Engine",
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr)
    {
        std::cerr << "Window creation failed! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

bool Window::Update()
{
    // Reset resize flag at the start of each frame
    wasResized = false;

    // Check for window resize
    int currentWidth, currentHeight;
    SDL_GetWindowSize(window, &currentWidth, &currentHeight);

    if (currentWidth != width || currentHeight != height)
    {
        width = currentWidth;
        height = currentHeight;
        wasResized = true;
        std::cout << "Window resized to: " << width << "x" << height << std::endl;
    }

    return true;
}

bool Window::PostUpdate()
{
    return true;
}

void Window::Render()
{
    SDL_GL_SwapWindow(window);
}

bool Window::CleanUp()
{
    std::cout << "Destroying SDL Window" << std::endl;
    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
    return true;
}

void Window::GetWindowSize(int& width, int& height) const
{
    width = this->width;
    height = this->height;
}

int Window::GetScale() const
{
    return scale;
}