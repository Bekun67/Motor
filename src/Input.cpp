#include "Input.h"
#include "Window.h"
#include "Application.h"

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
				//if we dropped a file
				std::string path(droppedFile);
				std::cout << "Dropped file: " << path << std::endl;

				//check the extension (case sensitive)
				std::string extension = "";
				if (path.size() >= 4) extension = path.substr(path.size() - 4);
				for (size_t i = 0; i < extension.size(); ++i) extension[i] = (char)tolower(extension[i]);

				//if the extension is fbx it means we have loaded a model
				if (extension == ".fbx") {
					//we add the new meshes and calculate the index from we have to draw the new game object
					size_t meshCountBefore = g_Meshes.size();

					if (LoadFile(path.c_str())) {
						std::cout << "FBX loaded" << std::endl;

						//aprox size to draw 
						float desiredSize = 5.0f; 
						float normalizeScale = (g_ModelRadius > 0.001f) ? (desiredSize / g_ModelRadius) : 1.0f;

						//create new game object with this mesh
						for (size_t i = meshCountBefore; i < g_Meshes.size(); ++i)
						{
							GameObject* go = new GameObject();
							go->name = "DroppedMesh_" + std::to_string(i);

							float offset = (float)(i - meshCountBefore) * desiredSize * 2.5f;
							go->transform->translation = aiVector3D(offset, 0.0f, 0.0f);
							go->transform->rotation = aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f);

							go->transform->scaling = aiVector3D(normalizeScale, normalizeScale, normalizeScale);

							go->mesh->meshIndex = (int)i;

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

bool Input::CleanUp()
{
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
	return windowEvents[ev];
}