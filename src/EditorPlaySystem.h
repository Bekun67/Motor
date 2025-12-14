#pragma once
#include <string>
#include <vector>
#include "GameObject.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class EditorPlaySystem
{
public:
    static void Init();
    static void Shutdown();

    static void Play();
    static void Stop();
    static void Pause();
    static void Step();

    static bool IsPlaying() { return m_IsPlaying; }
    static bool IsStopped() { return !m_IsPlaying && !m_IsPaused; }
    static bool IsPaused() { return m_IsPaused; }

private:
    static bool SerializeSceneState(const std::vector<GameObject*>& gameObjects);

    static bool RestoreSceneState(std::vector<GameObject*>& gameObjects);

    static bool m_IsPlaying;
    static bool m_IsPaused;

    static json m_SavedSceneState;
    static bool m_HasSavedState;
};
