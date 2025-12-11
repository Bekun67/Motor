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

    // Mark object as deleted but don't actually delete it
    m_Object->m_MarkedForDeletion = true;
}

void DeleteGameObjectCommand::Undo()
{
    if (!m_Object) return;

    // Unmark deletion
    m_Object->m_MarkedForDeletion = false;

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

// MultiTransformCommand implementation
MultiTransformCommand::MultiTransformCommand(const std::vector<GameObject*>& objects,
    const std::vector<glm::vec3>& oldPositions,
    const std::vector<glm::quat>& oldRotations,
    const std::vector<glm::vec3>& oldScales,
    const std::vector<glm::vec3>& newPositions,
    const std::vector<glm::quat>& newRotations,
    const std::vector<glm::vec3>& newScales)
    : m_Objects(objects)
    , m_OldPositions(oldPositions), m_NewPositions(newPositions)
    , m_OldRotations(oldRotations), m_NewRotations(newRotations)
    , m_OldScales(oldScales), m_NewScales(newScales)
{
}

void MultiTransformCommand::Execute()
{
    for (size_t i = 0; i < m_Objects.size(); ++i)
    {
        if (!m_Objects[i] || !m_Objects[i]->transform) continue;

        m_Objects[i]->transform->translation.x = m_NewPositions[i].x;
        m_Objects[i]->transform->translation.y = m_NewPositions[i].y;
        m_Objects[i]->transform->translation.z = m_NewPositions[i].z;

        m_Objects[i]->transform->rotation.w = m_NewRotations[i].w;
        m_Objects[i]->transform->rotation.x = m_NewRotations[i].x;
        m_Objects[i]->transform->rotation.y = m_NewRotations[i].y;
        m_Objects[i]->transform->rotation.z = m_NewRotations[i].z;

        m_Objects[i]->transform->scaling.x = m_NewScales[i].x;
        m_Objects[i]->transform->scaling.y = m_NewScales[i].y;
        m_Objects[i]->transform->scaling.z = m_NewScales[i].z;
    }
}

void MultiTransformCommand::Undo()
{
    for (size_t i = 0; i < m_Objects.size(); ++i)
    {
        if (!m_Objects[i] || !m_Objects[i]->transform) continue;

        m_Objects[i]->transform->translation.x = m_OldPositions[i].x;
        m_Objects[i]->transform->translation.y = m_OldPositions[i].y;
        m_Objects[i]->transform->translation.z = m_OldPositions[i].z;

        m_Objects[i]->transform->rotation.w = m_OldRotations[i].w;
        m_Objects[i]->transform->rotation.x = m_OldRotations[i].x;
        m_Objects[i]->transform->rotation.y = m_OldRotations[i].y;
        m_Objects[i]->transform->rotation.z = m_OldRotations[i].z;

        m_Objects[i]->transform->scaling.x = m_OldScales[i].x;
        m_Objects[i]->transform->scaling.y = m_OldScales[i].y;
        m_Objects[i]->transform->scaling.z = m_OldScales[i].z;
    }
}

std::string MultiTransformCommand::GetDescription() const
{
    return "Transform " + std::to_string(m_Objects.size()) + " objects";
}

// ReparentCommand implementation
ReparentCommand::ReparentCommand(GameObject* object, GameObject* oldParent, GameObject* newParent)
    : m_Object(object), m_OldParent(oldParent), m_NewParent(newParent)
{
}

void ReparentCommand::Execute()
{
    if (!m_Object) return;
    m_Object->SetParent(m_NewParent);
}

void ReparentCommand::Undo()
{
    if (!m_Object) return;
    m_Object->SetParent(m_OldParent);
}

std::string ReparentCommand::GetDescription() const
{
    return "Reparent " + (m_Object ? m_Object->name : "Unknown");
}

