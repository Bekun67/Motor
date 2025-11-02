#include "Input.h"
#include <iostream>
#include "Window.h"
#include <imgui_impl_sdl3.h>
#include "Application.h"


#define MAX_KEYS 300

Input::Input() : Module()
{
	name = "input";

	for (int i = 0; i < WE_COUNT; ++i)
	{
		windowEvents[i] = false;
	}

	keyboard = new KeyState[MAX_KEYS];
	// reserve memory
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

Input::~Input()
{
	delete[] keyboard;
}

// Called before render is available
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

// Called before the first frame
bool Input::Start()
{
	std::cout << "Init Input system" << std::endl;
	return true;
}

// Called each loop iteration
bool Input::PreUpdate()
{
	static SDL_Event event;

	mouseWheelX = 0;
	mouseWheelY = 0;

	SDL_Event event2;
	while (SDL_PollEvent(&event2))
	{
		// Let ImGui handle events first
		ImGui_ImplSDL3_ProcessEvent(&event2);

		switch (event2.type)
		{
		case SDL_EVENT_QUIT:
			windowEvents[WE_QUIT] = true;
			break;

		case SDL_EVENT_WINDOW_HIDDEN:
			windowEvents[WE_HIDE] = true;
			break;

		case SDL_EVENT_WINDOW_SHOWN:
			windowEvents[WE_SHOW] = true;
			break;

		case SDL_EVENT_MOUSE_WHEEL:
			mouseWheelY = (int)event2.wheel.y;
			break;
		}
	}

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
				std::string path(droppedFile);
				std::cout << "Dropped file: " << path << std::endl;

				std::string extension = "";
				if (path.size() >= 4) extension = path.substr(path.size() - 4);
				for (size_t i = 0; i < extension.size(); ++i) extension[i] = (char)tolower(extension[i]);

				if (extension == ".fbx") {
					if (LoadFile(path.c_str())) {
						std::cout << "Loaded FBX: " << path << std::endl;
					}
				}
			}
			break;
		}

		break;
		}
	}
	return true;
}

// Called before quitting
bool Input::CleanUp()
{
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
	return windowEvents[ev];
}

void Input::SetWindowEvent(EventWindow ev, bool value)
{
	windowEvents[ev] = value;
}

//Vector2D Input::GetMousePosition()
//{
//	return Vector2D(mouseX, mouseY);
//}
//
//Vector2D Input::GetMouseMotion()
//{
//	return Vector2D(mouseMotionX, mouseMotionY);
//}