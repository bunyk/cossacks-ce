#include <SDL3/SDL_timer.h>
#include "os.h"

int AddTime = 0;
int NeedAddTime = 0;

// This is mostly assigned to int or DWORD variables, which is fine as far as the game session takes less than 24 days
DLLEXPORT uint64_t GetSDLTickCount()
{
	return SDL_GetTicks();
}
