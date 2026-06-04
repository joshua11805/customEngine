#include "pch.h"
#include "Game.h"
#include "Profiler.h"
#define SDL_MAIN_USE_CALLBACKS 0  /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>	// SDL_main provides the main() function
#include <filesystem>

// Create a static instance of the Game class
static Game s_theGame;

/// This function runs once at startup.
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    std::filesystem::path cwd = std::filesystem::current_path();
    SDL_Log("Current working directory: %s", cwd.string().c_str());

    if (false == s_theGame.Init(800, 600))
    {
        DbgAssert(false, "Failed to init game");
        return SDL_APP_FAILURE;
    }

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/// This function runs when a new event (mouse input, keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	// Let ImGui see every event before the game handles it.
	s_theGame.ProcessEditorEvent(event);

	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
	}
	if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
	{
		s_theGame.OnResize(event->window.data1, event->window.data2);
	}
	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void* appstate)
{
	Profiler::Get()->ResetAll();

	s_theGame.ProcessInput();
	s_theGame.UpdateGame();
    s_theGame.RenderFrame();

	return SDL_APP_CONTINUE;  // carry on with the program!
}

/// This function runs once at shutdown.
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    s_theGame.Shutdown();
}
