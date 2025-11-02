#include "ImGuiModule.h"
#include "Application.h"
#include "EditorWindow.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <iostream>

ImGuiModule::ImGuiModule() : showAboutWindow(false)
{
    name = "ImGui";
}

ImGuiModule::~ImGuiModule()
{
}

bool ImGuiModule::Start()
{
    std::cout << "Initializing ImGui..." << std::endl;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    SDL_Window* window = Application::GetInstance().window->GetWindow();
    SDL_GLContext glContext = Application::GetInstance().opengl->glContext;

    if (!ImGui_ImplSDL3_InitForOpenGL(window, glContext))
    {
        std::cerr << "Failed to initialize ImGui SDL3 backend" << std::endl;
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330 core"))
    {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }

    std::cout << "ImGui initialized successfully" << std::endl;
    return true;
}

bool ImGuiModule::PreUpdate()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return true;
}

bool ImGuiModule::Update()
{
    ShowMainMenuBar();

    // Debug: verificar cuántas ventanas tenemos
    static bool logged = false;
    if (!logged)
    {
        std::cout << "ImGui Update - Number of editor windows: " << editorWindows.size() << std::endl;
        logged = true;
    }

    // Update all editor windows
    for (EditorWindow* window : editorWindows)
    {
        if (window && window->IsVisible())
        {
            window->Draw();
        }
    }

    // Show About window if requested
    if (showAboutWindow)
    {
        ShowAboutWindow();
    }

    return true;
}

bool ImGuiModule::PostUpdate()
{
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    return true;
}

bool ImGuiModule::CleanUp()
{
    std::cout << "Cleaning up ImGui..." << std::endl;

    // Cleanup editor windows
    for (EditorWindow* window : editorWindows)
    {
        delete window;
    }
    editorWindows.clear();

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    return true;
}

void ImGuiModule::AddEditorWindow(EditorWindow* window)
{
    if (window)
    {
        editorWindows.push_back(window);
    }
}

void ImGuiModule::ShowMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        ShowFileMenu();
        ShowViewMenu();
        ShowGeometryMenu();
        ShowHelpMenu();

        ImGui::EndMainMenuBar();
    }
}

void ImGuiModule::ShowFileMenu()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
            Application::GetInstance().input->SetWindowEvent(WE_QUIT, true);
        }
        ImGui::EndMenu();
    }
}

void ImGuiModule::ShowViewMenu()
{
    if (ImGui::BeginMenu("View"))
    {
        for (EditorWindow* window : editorWindows)
        {
            if (window)
            {
                bool visible = window->IsVisible();
                if (ImGui::MenuItem(window->GetName().c_str(), nullptr, &visible))
                {
                    window->SetVisible(visible);
                }
            }
        }
        ImGui::EndMenu();
    }
}

void ImGuiModule::ShowGeometryMenu()
{
    if (ImGui::BeginMenu("GameObject"))
    {
        if (ImGui::MenuItem("Create Cube"))
        {
            // TODO: Implement cube creation
            std::cout << "Creating Cube..." << std::endl;
        }
        if (ImGui::MenuItem("Create Sphere"))
        {
            // TODO: Implement sphere creation
            std::cout << "Creating Sphere..." << std::endl;
        }
        if (ImGui::MenuItem("Create Cylinder"))
        {
            // TODO: Implement cylinder creation
            std::cout << "Creating Cylinder..." << std::endl;
        }
        if (ImGui::MenuItem("Create Pyramid"))
        {
            // TODO: Implement pyramid creation
            std::cout << "Creating Pyramid..." << std::endl;
        }
        ImGui::EndMenu();
    }
}

void ImGuiModule::ShowHelpMenu()
{
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("Documentation"))
        {
            SDL_OpenURL("https://github.com/yourusername/yourrepo/docs");
        }
        if (ImGui::MenuItem("Report a Bug"))
        {
            SDL_OpenURL("https://github.com/yourusername/yourrepo/issues");
        }
        if (ImGui::MenuItem("Download Latest"))
        {
            SDL_OpenURL("https://github.com/yourusername/yourrepo/releases");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("About"))
        {
            showAboutWindow = !showAboutWindow;
        }
        ImGui::EndMenu();
    }
}

void ImGuiModule::ShowAboutWindow()
{
    if (!ImGui::Begin("About", &showAboutWindow))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Game Engine Motor");
    ImGui::Separator();
    ImGui::Text("Version: 1.0.0");
    ImGui::Spacing();

    ImGui::Text("Team Members:");
    ImGui::BulletText("Your Name Here");
    ImGui::Spacing();

    ImGui::Text("Libraries Used:");
    ImGui::BulletText("SDL3 %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    ImGui::BulletText("OpenGL 3.3");
    ImGui::BulletText("ImGui %s", IMGUI_VERSION);
    ImGui::BulletText("DevIL");
    ImGui::BulletText("Assimp");
    ImGui::BulletText("GLM");
    ImGui::Spacing();

    ImGui::Text("License:");
    ImGui::TextWrapped("MIT License - Copyright (c) 2025");
    ImGui::TextWrapped("Permission is hereby granted, free of charge, to any person obtaining a copy "
        "of this software and associated documentation files...");

    ImGui::End();
}