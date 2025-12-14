#include "Time.h"
#include <algorithm>

float Time::deltaTime = 0.0f;
float Time::time = 0.0f;
float Time::timeScale = 1.0f;
int Time::frameCount = 0;

float Time::realTimeSinceStartup = 0.0f;
float Time::realDeltaTime = 0.0f;

GameState Time::gameState = GameState::STOPPED;

std::chrono::high_resolution_clock::time_point Time::appStartTime;
std::chrono::high_resolution_clock::time_point Time::lastFrameTime;
std::chrono::high_resolution_clock::time_point Time::gameStartTime;

float Time::gameTimeAccumulator = 0.0f;
bool Time::stepOneFrame = false;

void Time::Init()
{
    appStartTime = std::chrono::high_resolution_clock::now();
    lastFrameTime = appStartTime;
    gameStartTime = appStartTime;

    deltaTime = 0.0f;
    time = 0.0f;
    timeScale = 1.0f;
    frameCount = 0;
    realTimeSinceStartup = 0.0f;
    realDeltaTime = 0.0f;
    gameTimeAccumulator = 0.0f;
    stepOneFrame = false;

    gameState = GameState::STOPPED;
}

void Time::Update()
{
    auto currentTime = std::chrono::high_resolution_clock::now();

    // Calculate real time
    std::chrono::duration<float> realDelta = currentTime - lastFrameTime;
    realDeltaTime = realDelta.count();

    std::chrono::duration<float> timeSinceStart = currentTime - appStartTime;
    realTimeSinceStartup = timeSinceStart.count();

    if (gameState == GameState::PLAYING)
    {
        float clampedTimeScale = std::clamp(timeScale, 0.0f, 10.0f);
        deltaTime = realDeltaTime * clampedTimeScale;

        gameTimeAccumulator += deltaTime;
        time = gameTimeAccumulator;
        frameCount++;
    }
    else if (gameState == GameState::PAUSED)
    {
        if (stepOneFrame)
        {
            float clampedTimeScale = std::clamp(timeScale, 0.0f, 10.0f);
            deltaTime = realDeltaTime * clampedTimeScale;

            gameTimeAccumulator += deltaTime;
            time = gameTimeAccumulator;
            frameCount++;

            stepOneFrame = false;
        }
        else
        {
            deltaTime = 0.0f;
        }
    }
    else // stop
    {
        deltaTime = 0.0f;
    }

    lastFrameTime = currentTime;
}

void Time::Play()
{
    if (gameState == GameState::STOPPED)
    {
        // Reset game clock
        gameStartTime = std::chrono::high_resolution_clock::now();
        gameTimeAccumulator = 0.0f;
        time = 0.0f;
        frameCount = 0;
    }

    gameState = GameState::PLAYING;
}

void Time::Stop()
{
    gameState = GameState::STOPPED;

    // Reset game time
    gameTimeAccumulator = 0.0f;
    time = 0.0f;
    deltaTime = 0.0f;
    frameCount = 0;
}

void Time::Pause()
{
    if (gameState == GameState::PLAYING)
    {
        gameState = GameState::PAUSED;
    }
}

void Time::Resume()
{
    if (gameState == GameState::PAUSED)
    {
        gameState = GameState::PLAYING;
    }
}

void Time::Step()
{
    if (gameState == GameState::PAUSED)
    {
        stepOneFrame = true;
    }
}