#include "HierarchyWindow.h"
#include "ModuleEditor.h"
#include "Application.h"
#include "Window.h"
#include "OpenGL.h"
#include "GameObject.h"
#include <algorithm>

HierarchyWindow::HierarchyWindow(ModuleEditor* editor)
    : EditorWindow(editor, "Hierarchy")
{
}

void HierarchyWindow::Draw()
{
    if (!visible) return;

    Window* window = Application::GetInstance().window.get();
    int windowWidth, windowHeight;
    window->GetWindowSize(windowWidth, windowHeight);

    if (editor->firstTimeSetup)
    {
        float x = editor->layout.marginX;
        float y = editor->layout.menuBarHeight + editor->layout.marginY;
        float width = windowWidth * editor->layout.hierarchyWidthPercent - editor->layout.marginX;
        float height = windowHeight * editor->layout.hierarchyHeightPercent - editor->layout.menuBarHeight - editor->layout.marginY * 2;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }
    else if (editor->useAdaptiveLayout)
    {
        float x = editor->layout.marginX;
        float y = editor->layout.menuBarHeight + editor->layout.marginY;
        float width = windowWidth * editor->layout.hierarchyWidthPercent - editor->layout.marginX;
        float height = windowHeight * editor->layout.hierarchyHeightPercent - editor->layout.menuBarHeight - editor->layout.marginY * 2;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    }

    ImGui::Begin("Hierarchy", &visible, ImGuiWindowFlags_HorizontalScrollbar);

    // Right-click on empty space to create empty GameObject
    if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty GameObject"))
        {
            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (opengl)
            {
                GameObject* emptyGO = new GameObject();
                int index = editor->CountNames("GameObject_");
                emptyGO->name = "GameObject_" + std::to_string(index);

                // Set default transform
                emptyGO->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
                emptyGO->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
                emptyGO->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

                // Set mesh index to -1 to indicate empty GameObject
                emptyGO->mesh->meshIndex = -1;
                emptyGO->meshPath = "";

                opengl->gameObjects.push_back(emptyGO);
                editor->sceneModified = true;

                LOG("Created empty GameObject: " + emptyGO->name);
            }
        }
        ImGui::EndPopup();
    }

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (opengl)
    {
        // Draw root level GameObjects (those without parent)
        for (GameObject* go : opengl->gameObjects)
        {
            if (go != nullptr && go->parent == nullptr)
            {
                DrawGameObjectNode(go);
            }
        }
    }

    // Handle drag & drop to empty space (unparent)
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DRAG"))
        {
            GameObject* draggedGO = *(GameObject**)payload->Data;
            if (draggedGO != nullptr)
            {
                draggedGO->SetParent(nullptr);
                editor->sceneModified = true;
                LOG("Unparented GameObject: " + draggedGO->name);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void HierarchyWindow::DrawGameObjectNode(GameObject* go)
{
    if (go == nullptr || go->m_MarkedForDeletion)
        return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (editor->IsSelected(go))
        flags |= ImGuiTreeNodeFlags_Selected;

    if (go->children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    std::string displayName = go->IsEmpty() ? "[E] " + go->name : go->name;

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)go->GetUUID(), flags, "%s", displayName.c_str());

    // Click: select with childs
    if (ImGui::IsItemClicked() && !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        editor->SelectGameObject(go, true);
    }

    // Double click: select just that object
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        editor->SelectGameObject(go, false);
        LOG("Double-clicked: Selecting only " + go->name);
    }

    // Context menu (right-click)
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Create Empty Child"))
        {
            GameObject* child = new GameObject(go);
            int index = editor->CountNames("GameObject_");
            child->name = "GameObject_" + std::to_string(index);

            child->transform->translation = aiVector3D(0.0f, 0.0f, 0.0f);
            child->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
            child->transform->scaling = aiVector3D(1.0f, 1.0f, 1.0f);

            child->mesh->meshIndex = -1;
            child->meshPath = "";

            OpenGL* opengl = Application::GetInstance().opengl.get();
            if (opengl)
            {
                opengl->gameObjects.push_back(child);
                editor->sceneModified = true;
                LOG("Created empty child GameObject: " + child->name);
            }
        }

        ImGui::Separator();

        if (go->parent != nullptr)
        {
            if (ImGui::MenuItem("Unparent", "Shift+P"))
            {
                go->SetParent(nullptr);
                editor->sceneModified = true;
                LOG("Unparented GameObject: " + go->name);
            }
        }

		ImGui::Separator();

        if (ImGui::MenuItem("Move Up"))
        {
            go->MoveUp();
            editor->sceneModified = true;
        }

        if (ImGui::MenuItem("Move Down"))
        {
            go->MoveDown();
            editor->sceneModified = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
        {
            // mark for deletion
            editor->MarkForDeletion(go);
        }

        ImGui::EndPopup();
    }

    // Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("GAMEOBJECT_DRAG", &go, sizeof(GameObject*));
        ImGui::Text("Move: %s", go->name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target
    HandleHierarchyDragDrop(go);

    if (nodeOpen)
    {
        // Create a copy 
        std::vector<GameObject*> childrenCopy = go->children;

        for (GameObject* child : childrenCopy)
        {
            if (child && !child->m_MarkedForDeletion)
            {
                DrawGameObjectNode(child);
            }
        }
        ImGui::TreePop();
    }
}

void HierarchyWindow::HandleHierarchyDragDrop(GameObject* target)
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_DRAG"))
        {
            GameObject* draggedGO = *(GameObject**)payload->Data;

            if (draggedGO != nullptr && draggedGO != target)
            {
                // Check that we're not trying to parent to a descendant
                bool isCyclic = false;
                GameObject* checkParent = target;
                while (checkParent != nullptr)
                {
                    if (checkParent == draggedGO)
                    {
                        isCyclic = true;
                        LOG_WARNING("Cannot parent GameObject to its own descendant!");
                        break;
                    }
                    checkParent = checkParent->parent;
                }

                if (!isCyclic)
                {
                    // Set new parent
                    draggedGO->SetParent(target);
                    editor->sceneModified = true;
                    LOG("Reparented " + draggedGO->name + " to " + target->name);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}