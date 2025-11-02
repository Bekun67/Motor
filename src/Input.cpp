#include "Input.h"
#include "Window.h"
#include "Application.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define MAX_KEYS 300

Input::Input() : Module()
{
	keyboard = new KeyState[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

Input::~Input()
{
	delete[] keyboard;
}

bool Input::Awake()
{
	bool ret = true;
	SDL_Init(0);

	if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0)
	{
		ret = false;
	}

	return ret;
}

bool Input::Start()
{
	return true;
}

bool Input::PreUpdate()
{
	static SDL_Event event;

	mouseWheelX = 0;
	mouseWheelY = 0;

	const bool* keys = SDL_GetKeyboardState(NULL);
	for (int i = 0; i < MAX_KEYS; ++i)
	{
		if (keys[i])
		{
			if (keyboard[i] == KEY_IDLE)
				keyboard[i] = KEY_DOWN;
			else
				keyboard[i] = KEY_REPEAT;
		}
		else
		{
			if (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN)
				keyboard[i] = KEY_UP;
			else
				keyboard[i] = KEY_IDLE;
		}
	}

	for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
	{
		if (mouseButtons[i] == KEY_DOWN)
			mouseButtons[i] = KEY_REPEAT;
		if (mouseButtons[i] == KEY_UP)
			mouseButtons[i] = KEY_IDLE;
	}

	OpenGL* opengl = Application::GetInstance().opengl.get();
	float scaleFactor = 0.1f;

	// GameObject selection with number keys 1-9
	if (keyboard[SDL_SCANCODE_1] == KEY_DOWN) {
		std::cout << "GameObject selected: 1" << std::endl;
		if (0 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[0];

	}

	if (keyboard[SDL_SCANCODE_2] == KEY_DOWN) {
		std::cout << "GameObject selected: 2 " << std::endl;
		if (1 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[1];
	}

	if (keyboard[SDL_SCANCODE_3] == KEY_DOWN) {
		std::cout << "GameObject selected: 3" << std::endl;
		if (1 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[1];
	}

	if (keyboard[SDL_SCANCODE_4] == KEY_DOWN) {
		std::cout << "GameObject selected: 4" << std::endl;
		if (1 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[1];
	}

	if (keyboard[SDL_SCANCODE_5] == KEY_DOWN) {
		std::cout << "GameObject selected: 5" << std::endl;
		if (1 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[1];
	}

	if (keyboard[SDL_SCANCODE_6] == KEY_DOWN) {
		std::cout << "GameObject selected: 6" << std::endl;
		if (1 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[1];
	}
	
	if (keyboard[SDL_SCANCODE_7] == KEY_DOWN) {
		std::cout << "GameObject selected: 7" << std::endl;
		if (1 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[1];
	}

	if (keyboard[SDL_SCANCODE_8] == KEY_DOWN) {
		std::cout << "GameObject selected: 8" << std::endl;
		if (1 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[1];
	}

	if (keyboard[SDL_SCANCODE_9] == KEY_DOWN) {
		std::cout << "GameObject selected: 9 " <<  std::endl;
		if (1 < opengl->gameObjects.size()) opengl->selectedGameObject = opengl->gameObjects[1];
	}

	// Transform manipulations
	if (opengl->selectedGameObject != nullptr)
	{
		ComponentTransform* transform = opengl->selectedGameObject->transform;

		if (transform != nullptr)
		{
			// Augment scale
			if (keyboard[SDL_SCANCODE_X])
			{
				transform->scaling.x += scaleFactor;
				transform->scaling.y += scaleFactor;
				transform->scaling.z += scaleFactor;
				std::cout << "Scaling up: " << transform->scaling.x << std::endl;
			}

			// less scale
			if (keys[SDL_SCANCODE_Z])
			{
				//Stop at 0.1
				float newScale = transform->scaling.x - scaleFactor;
				newScale = std::max(0.1f, newScale);

				transform->scaling.x = newScale;
				transform->scaling.y = newScale;
				transform->scaling.z = newScale;
				std::cout << "Scaling down: " << transform->scaling.x << std::endl;
			}

			float moveSpeed = 0.1f; 

	

			// Move X+
			if (keyboard[SDL_SCANCODE_L] != KEY_IDLE)
			{
				transform->translation.x += moveSpeed;
			}

			// Move x-
			if (keyboard[SDL_SCANCODE_J] != KEY_IDLE)
			{
				transform->translation.x -= moveSpeed;
			}

			// Move Z+
			if (keyboard[SDL_SCANCODE_I] != KEY_IDLE)
			{
				transform->translation.z += moveSpeed;
			}

			// Move z-
			if (keyboard[SDL_SCANCODE_K] != KEY_IDLE)
			{
				transform->translation.z -= moveSpeed;
			}

			// Move Y+
			if (keyboard[SDL_SCANCODE_O] != KEY_IDLE)
			{
				transform->translation.y += moveSpeed;
			}

			// Move Y-
			if (keyboard[SDL_SCANCODE_U] != KEY_IDLE)
			{
				transform->translation.y -= moveSpeed;
			}

			// Rotations
			aiQuaternion deltaRotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
			bool rotationApplied = false;

			float baseRotationSpeed = 0.9f;

			float deltaTime = 1.0f / 60.0f;

			float rotationValue = baseRotationSpeed * deltaTime;

			// Left and right rotation around Y axis (Yaw)
			if (keyboard[SDL_SCANCODE_B] != KEY_IDLE)
			{
				
				aiQuaternion rotB(rotationValue, 0.0f, 1.0f, 0.0f);
				deltaRotation = rotB * deltaRotation; // Premultiply for local space rotation
				rotationApplied = true;
			}
			if (keyboard[SDL_SCANCODE_V] != KEY_IDLE)
			{

				aiQuaternion rotV(-rotationValue, 0.0f, 1.0f, 0.0f);
				deltaRotation = rotV * deltaRotation;
				rotationApplied = true;
			}

			// Front and back rotation around X axis (Pitch)
			if (keyboard[SDL_SCANCODE_M] != KEY_IDLE)
			{

				aiQuaternion rotM(rotationValue, 1.0f, 0.0f, 0.0f);
				deltaRotation = rotM * deltaRotation;
				rotationApplied = true;
			}
			if (keyboard[SDL_SCANCODE_N] != KEY_IDLE)
			{

				aiQuaternion rotN(-rotationValue, 1.0f, 0.0f, 0.0f);
				deltaRotation = rotN * deltaRotation;
				rotationApplied = true;
			}

			// Apply acomulated rotation to the transform
			if (rotationApplied)
			{
				transform->rotation = deltaRotation * transform->rotation;

				transform->rotation.Normalize();
			}
		}
	}

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			windowEvents[WE_QUIT] = true;
			break;
		case SDL_EVENT_WINDOW_HIDDEN:
		case SDL_EVENT_WINDOW_MINIMIZED:
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			windowEvents[WE_HIDE] = true;
			break;
		case SDL_EVENT_WINDOW_SHOWN:
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		case SDL_EVENT_WINDOW_MAXIMIZED:
		case SDL_EVENT_WINDOW_RESTORED:
			windowEvents[WE_SHOW] = true;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			mouseButtons[event.button.button - 1] = KEY_DOWN;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			mouseButtons[event.button.button - 1] = KEY_UP;
			break;
		case SDL_EVENT_MOUSE_MOTION:
		{
			int scale = Application::GetInstance().window.get()->GetScale();
			mouseMotionX = event.motion.xrel / scale;
			mouseMotionY = event.motion.yrel / scale;
			mouseX = event.motion.x / scale;
			mouseY = event.motion.y / scale;
			break;
		}
		case SDL_EVENT_MOUSE_WHEEL:
		{
			mouseWheelX = event.wheel.x;
			mouseWheelY = event.wheel.y;
			break;
		}
		case SDL_EVENT_DROP_FILE:
		{
			SDL_DropEvent drop = event.drop;
			const char* droppedFile = drop.data;

			if (droppedFile) {
				std::string path(droppedFile);
				std::cout << "Dropped file: " << path << std::endl;

				//check for extension
				std::string extension = "";
				if (path.size() >= 4) extension = path.substr(path.size() - 4);
				for (size_t i = 0; i < extension.size(); ++i) extension[i] = (char)tolower(extension[i]);

				if (extension == ".fbx") {
					size_t meshCountBefore = g_Meshes.size();

					if (LoadFile(path.c_str())) {
						std::cout << "FBX loaded" << std::endl;

						float desiredSize = 5.0f;
						float normalizeScale = (g_ModelRadius > 0.001f) ? (desiredSize / g_ModelRadius) : 1.0f;

						// Obtener posición del ratón
						float  mouseX, mouseY;
						SDL_GetMouseState(&mouseX, &mouseY);

						// Obtener cámara
						Camera* camera = &(Application::GetInstance().opengl->camera);

						// Obtener viewport
						int viewport[4];
						glGetIntegerv(GL_VIEWPORT, viewport);

						// Convertir coordenadas de pantalla a NDC
						float x = (2.0f * mouseX) / viewport[2] - 1.0f;
						float y = 1.0f - (2.0f * mouseY) / viewport[3];

						// Crear ray desde la cámara
						glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
						glm::vec4 rayEye = glm::inverse(camera->GetProjectionMatrix()) * rayClip;
						rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
						glm::vec3 rayWorld = glm::vec3(glm::inverse(camera->GetViewMatrix()) * rayEye);
						rayWorld = glm::normalize(rayWorld);

						// Intersección con plano Y=0
						glm::vec3 camPos = camera->GetPosition();
						float t = -camPos.y / rayWorld.y;
						glm::vec3 dropPosition = camPos + rayWorld * t;

						for (size_t i = meshCountBefore; i < g_Meshes.size(); ++i)
						{
							GameObject* go = new GameObject();
							go->name = "DroppedMesh_" + std::to_string(i);

							// Usar la posición calculada en lugar del offset
							go->transform->translation = aiVector3D(dropPosition.x, 0.0f, dropPosition.z);
							go->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
							go->transform->scaling = aiVector3D(normalizeScale, normalizeScale, normalizeScale);

							go->mesh->meshIndex = (int)i;

							// Resto del código de texturas...
							std::string texturePath = GetTexturePathFromFBX(path.c_str(), (int)(i - meshCountBefore));

							bool textureLoaded = false;
							if (!texturePath.empty()) {
								textureLoaded = go->texture->LoadTexture(texturePath);
							}

							if (textureLoaded) {
								std::cout << "Assigned texture from FBX: " << texturePath << std::endl;
							}
							else {
								std::cout << "No valid texture found, using checkerboard" << std::endl;

								const int size = 64;
								GLubyte checkerImage[64][64][4];
								for (int i = 0; i < size; i++) {
									for (int j = 0; j < size; j++) {
										int c = ((((i & 0x8) == 0) ^ ((j & 0x8) == 0))) * 255;
										checkerImage[i][j][0] = (GLubyte)c;
										checkerImage[i][j][1] = (GLubyte)c;
										checkerImage[i][j][2] = (GLubyte)c;
										checkerImage[i][j][3] = (GLubyte)255;
									}
								}

								GLuint checkID;
								glGenTextures(1, &checkID);
								glBindTexture(GL_TEXTURE_2D, checkID);
								glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);
								glGenerateMipmap(GL_TEXTURE_2D);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
								glBindTexture(GL_TEXTURE_2D, 0);

								go->texture->hasTexture = true;
								if (go->texture->texturedata == nullptr) {
									go->texture->texturedata = new TextureData();
								}
								go->texture->texturedata->id = checkID;
								go->texture->texturedata->type = "checkerboard";
								go->texture->texturedata->path = "checkerboard";

								std::cout << "Checkerboard created with ID: " << checkID << std::endl;
							}

							Application::GetInstance().opengl->gameObjects.push_back(go);
							std::cout << "Created GameObject " << go->name << std::endl;
						}

						std::cout << "Total GameObjects in scene: " << Application::GetInstance().opengl->gameObjects.size() << std::endl;
					}
				}
			}
			break;
		}
		}
	}
	return true;
}

std::string Input::GetTexturePathFromFBX(const char* fbxPath, int meshIndex)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fbxPath,
		aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || meshIndex >= (int)scene->mNumMeshes) {
		return "";
	}

	aiMesh* mesh = scene->mMeshes[meshIndex];
	if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < (int)scene->mNumMaterials) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString texPath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

			std::string fullPath = texPath.C_Str();

			//get the directory
			std::string fbxDir = fbxPath;
			size_t lastSlash = fbxDir.find_last_of("/\\");
			if (lastSlash != std::string::npos) {
				fbxDir = fbxDir.substr(0, lastSlash + 1);
			}

			if (fullPath.find(":") == std::string::npos &&
				fullPath[0] != '/' && fullPath[0] != '\\') {
				fullPath = fbxDir + fullPath;
			}

			return fullPath;
		}
	}

	return "";
}

bool Input::CleanUp()
{
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
	return windowEvents[ev];
}