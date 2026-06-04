#include "pch.h"
#ifdef _DEBUG

// Implements custom assert handler
#include "Renderer.h"
#include <assert.h>
#include <cstdlib>
bool DbgAssertFunction(bool expr, const char* expr_string, const char* desc, int line_num, const char* file_name)
{
	bool bShouldHalt = !expr;
	if (bShouldHalt)
	{
		static char szBuffer[1024];
		SDL_snprintf(szBuffer, 1024,
					"Assertion Failed!\n\nDescription: %s\nExpression: %s\nFile: %s\nLine: %d\n",
					desc, expr_string, file_name, line_num);
		Renderer* renderer = Renderer::Get();
		if (renderer)
		{
			SDL_Window* window = renderer->GetWindow();
			if (window)
			{
				SDL_MessageBoxButtonData buttons[] = {
					{SDL_MESSAGEBOX_ERROR, 0, "Debug/Stop"},
					{SDL_MESSAGEBOX_ERROR, 1, "Ignore"}
				};
				SDL_MessageBoxData messageData = {
					SDL_MESSAGEBOX_ERROR,
					window,
					"Assert",
					szBuffer,
					2,
					buttons,
					nullptr
				};
				int button = 0;
				SDL_ShowMessageBox(&messageData, &button);
				return button == 0;
//				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Assert", szBuffer, Renderer::Get()->GetWindow());
				return true;
			}
		}
		SDL_Log(szBuffer);
		SDL_TriggerBreakpoint();
	}

	return bShouldHalt;
}

#endif