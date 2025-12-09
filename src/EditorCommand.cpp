#include "EditorCommand.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "Application.h"
#include "OpenGL.h"
#include "ModuleEditor.h"
#include <algorithm>

// TransformCommand implementation
TransformCommand::TransformCommand(GameObject* object,
    const glm::vec3& oldPos, const glm::quat& oldRot, const glm::vec3& oldScale,
    const glm::vec3& newPos, const glm::quat& newRot, const glm::vec3& newScale)
    : m_Object(object)
    , m_OldPosition(oldPos), m_NewPosition(newPos)
    , m_OldRotation(oldRot), m_NewRotation(newRot)
    , m_OldScale(oldScale), m_NewScale(newScale)
{
}

void TransformCommand::Execute()
{
    if (!m_Object || !m_Object->transform) return;

    m_Object->transform->translation.x = m_NewPosition.x;
    m_Object->transform->translation.y = m_NewPosition.y;
    m_Object->transform->translation.z = m_NewPosition.z;

    m_Object->transform->rotation.w = m_NewRotation.w;
    m_Object->transform->rotation.x = m_NewRotation.x;
    m_Object->transform->rotation.y = m_NewRotation.y;
    m_Object->transform->rotation.z = m_NewRotation.z;

    m_Object->transform->scaling.x = m_NewScale.x;
    m_Object->transform->scaling.y = m_NewScale.y;
    m_Object->transform->scaling.z = m_NewScale.z;
}

void TransformCommand::Undo()
{
    if (!m_Object || !m_Object->transform) return;

    m_Object->transform->translation.x = m_OldPosition.x;
    m_Object->transform->translation.y = m_OldPosition.y;
    m_Object->transform->translation.z = m_OldPosition.z;

    m_Object->transform->rotation.w = m_OldRotation.w;
    m_Object->transform->rotation.x = m_OldRotation.x;
    m_Object->transform->rotation.y = m_OldRotation.y;
    m_Object->transform->rotation.z = m_OldRotation.z;

    m_Object->transform->scaling.x = m_OldScale.x;
    m_Object->transform->scaling.y = m_OldScale.y;
    m_Object->transform->scaling.z = m_OldScale.z;
}

std::string TransformCommand::GetDescription() const
{
    return "Transform " + (m_Object ? m_Object->name : "Unknown");
}

// CreateGameObjectCommand implementation
CreateGameObjectCommand::CreateGameObjectCommand(GameObject* object)
    : m_Object(object), m_IsDeleted(false)
{
}

void CreateGameObjectCommand::Execute()
{
    if (m_IsDeleted)
    {
        // Re-add to scene
        OpenGL* opengl = Application::GetInstance().opengl.get();
        if (opengl)
        {
            opengl->gameObjects.push_back(m_Object);
            m_IsDeleted = false;
        }
    }
}

void CreateGameObjectCommand::Undo()
{
    if (!m_IsDeleted)
    {
        // Remove from scene without deleting
        OpenGL* opengl = Application::GetInstance().opengl.get();
        if (opengl)
        {
            auto it = std::find(opengl->gameObjects.begin(), opengl->gameObjects.end(), m_Object);
            if (it != opengl->gameObjects.end())
            {
                opengl->gameObjects.erase(it);
                m_IsDeleted = true;
            }
        }
    }
}

std::string CreateGameObjectCommand::GetDescription() const
{
    return "Create " + (m_Object ? m_Object->name : "Unknown");
}

// DeleteGameObjectCommand implementation
DeleteGameObjectCommand::DeleteGameObjectCommand(GameObject* object)
    : m_Object(object)
    , m_Parent(object ? object->parent : nullptr)
    , m_IndexInParent(0)
    , m_WasInScene(false)
{
    if (m_Parent)
    {
        auto it = std::find(m_Parent->children.begin(), m_Parent->children.end(), m_Object);
        if (it != m_Parent->children.end())
        {
            m_IndexInParent = std::distance(m_Parent->children.begin(), it);
        }
    }

    if (m_Object)
    {
        m_Children = m_Object->children;
    }
}

void DeleteGameObjectCommand::Execute()
{
    if (!m_Object) return;

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (opengl)
    {
        auto it = std::find(opengl->gameObjects.begin(), opengl->gameObjects.end(), m_Object);
        if (it != opengl->gameObjects.end())
        {
            opengl->gameObjects.erase(it);
            m_WasInScene = true;
        }
    }

    if (m_Parent)
    {
        m_Parent->RemoveChild(m_Object);
    }
}

void DeleteGameObjectCommand::Undo()
{
    if (!m_Object) return;

    if (m_WasInScene)
    {
        OpenGL* opengl = Application::GetInstance().opengl.get();
        if (opengl)
        {
            opengl->gameObjects.push_back(m_Object);
        }
    }

    if (m_Parent)
    {
        m_Object->SetParent(m_Parent);
    }

    // Restore children
    for (GameObject* child : m_Children)
    {
        child->SetParent(m_Object);
    }
}

std::string DeleteGameObjectCommand::GetDescription() const
{
    return "Delete " + (m_Object ? m_Object->name : "Unknown");
}

// CommandHistory implementation
CommandHistory::CommandHistory(size_t maxHistory)
    : m_CurrentIndex(0), m_MaxHistory(maxHistory)
{
}

void CommandHistory::ExecuteCommand(std::unique_ptr<EditorCommand> command)
{
    // Remove any commands after current index (redo history)
    while (m_History.size() > m_CurrentIndex)
    {
        m_History.pop_back();
    }

    // Execute the command
    command->Execute();

    // Add to history
    m_History.push_back(std::move(command));
    m_CurrentIndex++;

    // Limit history size
    while (m_History.size() > m_MaxHistory)
    {
        m_History.pop_front();
        m_CurrentIndex--;
    }
}

bool CommandHistory::CanUndo() const
{
    return m_CurrentIndex > 0;
}

bool CommandHistory::CanRedo() const
{
    return m_CurrentIndex < m_History.size();
}

void CommandHistory::Undo()
{
    if (!CanUndo()) return;

    m_CurrentIndex--;
    m_History[m_CurrentIndex]->Undo();

    LOG("Undo: " + m_History[m_CurrentIndex]->GetDescription());
}

void CommandHistory::Redo()
{
    if (!CanRedo()) return;

    m_History[m_CurrentIndex]->Execute();
    m_CurrentIndex++;

    LOG("Redo: " + m_History[m_CurrentIndex - 1]->GetDescription());
}

void CommandHistory::Clear()
{
    m_History.clear();
    m_CurrentIndex = 0;
}

const std::string& CommandHistory::GetUndoDescription() const
{
    static std::string empty = "";
    if (!CanUndo()) return empty;
    return m_History[m_CurrentIndex - 1]->GetDescription();
}

const std::string& CommandHistory::GetRedoDescription() const
{
    static std::string empty = "";
    if (!CanRedo()) return empty;
    return m_History[m_CurrentIndex]->GetDescription();
}