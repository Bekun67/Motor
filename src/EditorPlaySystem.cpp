#include "EditorPlaySystem.h"
#include "SceneSerializer.h"
#include "Time.h"
#include "Application.h"
#include "OpenGL.h"
#include "ModuleEditor.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include <iostream>

bool EditorPlaySystem::m_IsPlaying = false;
bool EditorPlaySystem::m_IsPaused = false;
json EditorPlaySystem::m_SavedSceneState;
bool EditorPlaySystem::m_HasSavedState = false;

void EditorPlaySystem::Init()
{
    m_IsPlaying = false;
    m_IsPaused = false;
    m_HasSavedState = false;
    m_SavedSceneState.clear();
}

void EditorPlaySystem::Shutdown()
{
    m_SavedSceneState.clear();
    m_HasSavedState = false;
}

void EditorPlaySystem::Play()
{
    if (m_IsPlaying)
    {
        if (m_IsPaused)
        {
            m_IsPaused = false;
            Time::Resume();
            LOG("Game resumed");
        }
        return;
    }

    LOG("=== PLAY - Saving scene state ===");

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
    {
        LOG_ERROR("Failed to get OpenGL module");
        return;
    }

    if (!SerializeSceneState(opengl->gameObjects))
    {
        LOG_ERROR("Failed to serialize scene state");
        return;
    }

    m_IsPlaying = true;
    m_IsPaused = false;
    m_HasSavedState = true;

    Time::Play();

    LOG("Game started - Scene state saved");
    LOG("GameObjects in scene: " + std::to_string(opengl->gameObjects.size()));
}

void EditorPlaySystem::Stop()
{
    if (!m_IsPlaying)
        return;

    LOG("=== STOP - Restoring scene state ===");

    Time::Stop();

    OpenGL* opengl = Application::GetInstance().opengl.get();
    if (!opengl)
    {
        LOG_ERROR("Failed to get OpenGL module");
        return;
    }

    ModuleEditor* editor = Application::GetInstance().editor.get();
    if (editor)
    {
        editor->DeselectAll();
    }
    opengl->selectedGameObject = nullptr;

    if (m_HasSavedState)
    {
        if (!RestoreSceneState(opengl->gameObjects))
        {
            LOG_ERROR("Failed to restore scene state");
        }
        else
        {
            LOG("Scene restored successfully");
        }
    }
    else
    {
        LOG_WARNING("No saved state to restore");
    }

    m_IsPlaying = false;
    m_IsPaused = false;
    m_HasSavedState = false;
    m_SavedSceneState.clear();

    LOG("Game stopped");
}

void EditorPlaySystem::Pause()
{
    if (!m_IsPlaying || m_IsPaused)
        return;

    m_IsPaused = true;
    Time::Pause();
    LOG("Game paused");
}

void EditorPlaySystem::Step()
{
    if (!m_IsPaused)
        return;

    Time::Step();
    LOG("Game stepped one frame");
}

bool EditorPlaySystem::SerializeSceneState(const std::vector<GameObject*>& gameObjects)
{
    try
    {
        m_SavedSceneState = json::object();
        m_SavedSceneState["GameObjects"] = json::array();

        for (const GameObject* go : gameObjects)
        {
            if (go != nullptr)
            {
                json goJson;

                goJson["UUID"] = go->GetUUID().ToString();
                goJson["ParentUUID"] = go->GetParentUUID().ToString();
                goJson["Name"] = go->name;
                goJson["Active"] = go->active;
                goJson["MeshPath"] = go->meshPath;
                goJson["IsEmpty"] = go->IsEmpty();
                goJson["MeshIndexInFBX"] = go->meshIndexInFBX;
                goJson["IsStatic"] = go->isStatic;

                if (go->transform)
                {
                    PropertyMap transformProps = go->transform->Serialize();
                    json transformJson;
                    for (const auto& [key, value] : transformProps)
                    {
                        if (std::holds_alternative<float>(value))
                            transformJson[key] = std::get<float>(value);
                        else if (std::holds_alternative<double>(value))
                            transformJson[key] = std::get<double>(value);
                        else if (std::holds_alternative<int>(value))
                            transformJson[key] = std::get<int>(value);
                        else if (std::holds_alternative<bool>(value))
                            transformJson[key] = std::get<bool>(value);
                    }
                    goJson["Transform"] = transformJson;
                }

                if (go->mesh)
                {
                    PropertyMap meshProps = go->mesh->Serialize();
                    json meshJson;
                    for (const auto& [key, value] : meshProps)
                    {
                        if (std::holds_alternative<int>(value))
                            meshJson[key] = std::get<int>(value);
                        else if (std::holds_alternative<bool>(value))
                            meshJson[key] = std::get<bool>(value);
                    }
                    goJson["Mesh"] = meshJson;
                }

                if (go->texture && !go->IsEmpty())
                {
                    PropertyMap textureProps = go->texture->Serialize();
                    json textureJson;
                    for (const auto& [key, value] : textureProps)
                    {
                        if (std::holds_alternative<std::string>(value))
                            textureJson[key] = std::get<std::string>(value);
                        else if (std::holds_alternative<bool>(value))
                            textureJson[key] = std::get<bool>(value);
                    }
                    goJson["Texture"] = textureJson;
                }

                m_SavedSceneState["GameObjects"].push_back(goJson);
            }
        }

        LOG("Scene serialized: " + std::to_string(gameObjects.size()) + " GameObjects");
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception during serialization: " + std::string(e.what()));
        return false;
    }
}

bool EditorPlaySystem::RestoreSceneState(std::vector<GameObject*>& gameObjects)
{
    try
    {
        for (GameObject* go : gameObjects)
        {
            if (go != nullptr)
            {
                go->parent = nullptr;
                go->children.clear();
            }
        }

        for (GameObject* go : gameObjects)
        {
            if (go != nullptr)
            {
                go->m_IsBeingDestroyed = true;
                delete go;
            }
        }

        gameObjects.clear();

        if (!m_SavedSceneState.contains("GameObjects"))
        {
            LOG_ERROR("No GameObjects in saved state");
            return false;
        }

        std::map<uint32_t, GameObject*> uuidMap;

        for (const auto& goJson : m_SavedSceneState["GameObjects"])
        {
            GameObject* go = new GameObject();

            //UUID
            if (goJson.contains("UUID"))
            {
                std::string uuidStr = goJson["UUID"];
                go->SetUUID(EngineUUID::FromString(uuidStr));
            }

            if (goJson.contains("ParentUUID"))
            {
                std::string parentUuidStr = goJson["ParentUUID"];
                go->SetParentUUID(EngineUUID::FromString(parentUuidStr));
            }

            if (goJson.contains("Name"))
                go->name = goJson["Name"];

            if (goJson.contains("Active"))
                go->active = goJson["Active"];

            if (goJson.contains("MeshPath"))
                go->meshPath = goJson["MeshPath"];

            if (goJson.contains("MeshIndexInFBX"))
                go->meshIndexInFBX = goJson["MeshIndexInFBX"];

            if (goJson.contains("IsStatic"))
                go->isStatic = goJson["IsStatic"];

            if (goJson.contains("Transform") && go->transform)
            {
                PropertyMap transformProps;
                for (auto& [key, value] : goJson["Transform"].items())
                {
                    if (value.is_number_float())
                        transformProps[key] = value.get<float>();
                    else if (value.is_number_integer())
                        transformProps[key] = value.get<int>();
                    else if (value.is_boolean())
                        transformProps[key] = value.get<bool>();
                }
                go->transform->Deserialize(transformProps);
            }

            if (goJson.contains("Mesh") && go->mesh)
            {
                PropertyMap meshProps;
                for (auto& [key, value] : goJson["Mesh"].items())
                {
                    if (value.is_number_integer())
                        meshProps[key] = value.get<int>();
                    else if (value.is_boolean())
                        meshProps[key] = value.get<bool>();
                }
                go->mesh->Deserialize(meshProps);
            }

            if (goJson.contains("Texture") && go->texture)
            {
                PropertyMap textureProps;
                for (auto& [key, value] : goJson["Texture"].items())
                {
                    if (value.is_string())
                        textureProps[key] = value.get<std::string>();
                    else if (value.is_boolean())
                        textureProps[key] = value.get<bool>();
                }
                go->texture->Deserialize(textureProps);
            }

            gameObjects.push_back(go);
            uuidMap[(uint32_t)go->GetUUID()] = go;
        }

        for (GameObject* go : gameObjects)
        {
            uint32_t parentUUID = (uint32_t)go->GetParentUUID();
            if (parentUUID != 0 && uuidMap.count(parentUUID))
            {
                GameObject* parentGo = uuidMap[parentUUID];
                go->parent = parentGo;
                parentGo->children.push_back(go);
            }
            else
            {
                go->parent = nullptr;
            }
        }

        LOG("Scene restored: " + std::to_string(gameObjects.size()) + " GameObjects");
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception during restoration: " + std::string(e.what()));
        return false;
    }
}