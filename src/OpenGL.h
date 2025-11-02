#pragma once
#include "Module.h"
#include "Camera.h"
#include <SDL3/SDL.h>
#include <vector>

class GameObject;
struct SDL_Window;

class OpenGL : public Module
{
public:
	OpenGL();
	~OpenGL();

	SDL_GLContext glContext;
	unsigned int shaderProgram;
	unsigned int VAO;
	unsigned int VBO;

	bool showGrid = true;
	unsigned int gridVAO = 0;
	unsigned int gridVBO = 0;
	int gridLineCount = 0;

	// GameObjects in scene
	std::vector<GameObject*> gameObjects;

private:
	bool Start() override;
	bool CleanUp() override;
	bool Update() override;
	bool Draw();

	void CreateGrid(int size);
	void DrawGrid();

	Camera camera;
	uint64_t lastTicks;

	float scaleFactor = 1.0f;
};