#pragma once
#include <memory>
#include <vector>
#include <deque>
#include <string>
#include <glm/gtc/quaternion.hpp>

// Base class for all editor commands
class EditorCommand
{
public:
    virtual ~EditorCommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

// Command for transform changes
class TransformCommand : public EditorCommand
{
public:
    TransformCommand(class GameObject* object,
        const glm::vec3& oldPos, const glm::quat& oldRot, const glm::vec3& oldScale,
        const glm::vec3& newPos, const glm::quat& newRot, const glm::vec3& newScale);

    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    GameObject* m_Object;
    glm::vec3 m_OldPosition, m_NewPosition;
    glm::quat m_OldRotation, m_NewRotation;
    glm::vec3 m_OldScale, m_NewScale;
};

// Command for GameObject creation
class CreateGameObjectCommand : public EditorCommand
{
public:
    CreateGameObjectCommand(GameObject* object);

    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    GameObject* m_Object;
    bool m_IsDeleted;
};

// Command for GameObject deletion
class DeleteGameObjectCommand : public EditorCommand
{
public:
    DeleteGameObjectCommand(GameObject* object);

    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    GameObject* m_Object;
    GameObject* m_Parent;
    std::vector<GameObject*> m_Children;
    size_t m_IndexInParent;
    bool m_WasInScene;
};

// Command history manager
class CommandHistory
{
public:
    CommandHistory(size_t maxHistory = 100);

    void ExecuteCommand(std::unique_ptr<EditorCommand> command);
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();
    void Clear();

    const std::string& GetUndoDescription() const;
    const std::string& GetRedoDescription() const;

private:
    std::deque<std::unique_ptr<EditorCommand>> m_History;
    size_t m_CurrentIndex;
    size_t m_MaxHistory;
};