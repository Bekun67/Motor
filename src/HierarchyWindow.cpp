#include "HierarchyWindow.h"
#include "Application.h"
#include "GameObject.h"
#include <imgui.h>

GameObject* HierarchyWindow::selectedGameObject = nullptr;

HierarchyWindow::HierarchyWindow()
    : EditorWindow("Hierarchy", true)
{
}

HierarchyWindow::~HierarchyWindow()
{
}

void HierarchyWindow::Draw()
{
    if (!ImGui::Begin(name.c_str(), &visible))
    {
        ImGui::End();
        return;
    }

    // Get all GameObjects from OpenGL module
    std::vector<GameObject*>& gameObjects = Application::GetInstance().opengl->gameObjects;

    if (gameObjects.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No GameObjects in scene");
    }
    else
    {
        for (GameObject* go : gameObjects)
        {
            if (go && go->parent == nullptr) // Only show root objects
            {
                DrawGameObjectNode(go);
            }
        }
    }

    // Click on empty space to deselect
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
    {
        selectedGameObject = nullptr;
    }

    ImGui::End();
}

void HierarchyWindow::DrawGameObjectNode(GameObject* go)
{
    if (!go) return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    // Check if selected
    if (go == selectedGameObject)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    // Check if has children
    if (go->children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool nodeOpen = ImGui::TreeNodeEx(go->name.c_str(), flags);

    // Handle selection
    if (ImGui::IsItemClicked())
    {
        selectedGameObject = go;
    }

    // Draw children
    if (nodeOpen && !go->children.empty())
    {
        for (GameObject* child : go->children)
        {
            DrawGameObjectNode(child);
        }
        ImGui::TreePop();
    }
}