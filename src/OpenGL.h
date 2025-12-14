#pragma once
#include "Module.h"
#include "Camera.h"
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include "Quadtree.h"
#include "ModuleEditor.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

class GameObject;
struct SDL_Window;

class Model;

class OpenGL : public Module
{
public:
	OpenGL();
	~OpenGL();

	SDL_GLContext glContext;
	unsigned int shaderProgram;
	unsigned int VAO;
	unsigned int VBO;

	Model* fbxModel = nullptr;
	bool showGrid = true;
	unsigned int gridVAO = 0;
	unsigned int normalShaderProgram;
	unsigned int edgeDetectionShader = 0;
	unsigned int maskShader = 0;
	unsigned int gridVBO = 0;
	int gridLineCount = 0;

	//GameObjects in scene
	std::vector<GameObject*> gameObjects;

	GameObject* selectedGameObject = nullptr;

	void* GetGLContext() const { return glContext; }

	static OpenGL& GetInstance();
	Camera camera;

	char *buf;
	float f;

	//frustum culling stats
	int culledCount = 0;
	int renderedCount = 0;

	unsigned int depthDebugShader = 0;
	unsigned int fullscreenQuadVAO = 0;
	unsigned int fullscreenQuadVBO = 0;

	bool debugZBuffer = false;
	unsigned int fbo = 0;
	unsigned int depthTexture = 0;
	unsigned int colorTexture = 0;
	int depthTextureWidth = 0;
	int depthTextureHeight = 0;

	unsigned int selectionFBO = 0;
	unsigned int selectionTexture = 0;
	unsigned int selectionDepthBuffer = 0;
	int selectionTextureWidth = 0;
	int selectionTextureHeight = 0;

	glm::vec3 debugRayStart;
	glm::vec3 debugRayEnd;

	//quadtree
	Quadtree quadtree;
	bool useQuadtree = false;
	bool showQuadtree = false;
	void RebuildQuadtree();
	int quadtreeTestsCount = 0;
	int quadtreeCulledCount = 0;
	bool EmptyQuadtree();
	bool extraQuadtreeInfo = false;

private:
	bool Start() override;
	bool CleanUp() override;
	bool Update() override;
	bool Draw();

	void CreateGrid(int size);
	void DrawGrid();

	uint64_t lastTicks;

	float scaleFactor = 1.0f;
	ModuleEditor* editor;
};