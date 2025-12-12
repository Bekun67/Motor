#pragma once

#include <chrono>

enum class GameState
{
    STOPPED,
    PLAYING,
    PAUSED
};

class Time
{
public:
    static void Init();
    static void Update();

    // Editor 
    static void Play();
    static void Stop();
    static void Pause();
    static void Resume();
    static void Step(); 

    // Game Clock 
    static float deltaTime;           
    static float time;                
    static float timeScale;           
    static int frameCount;            

    // Real Time Clock 
    static float realTimeSinceStartup;  
    static float realDeltaTime;         

    static GameState gameState;
    static bool IsPaused() { return gameState == GameState::PAUSED; }
    static bool IsPlaying() { return gameState == GameState::PLAYING; }
    static bool IsStopped() { return gameState == GameState::STOPPED; }

private:
    static std::chrono::high_resolution_clock::time_point appStartTime;
    static std::chrono::high_resolution_clock::time_point lastFrameTime;
    static std::chrono::high_resolution_clock::time_point gameStartTime;

    static float gameTimeAccumulator; 
    static bool stepOneFrame;         
};