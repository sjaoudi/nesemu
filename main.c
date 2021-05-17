#include "cartridge.h"
#include "system.h"
#include "video.h"
#include "controller.h"
#include "cpu.h"
#include "memory.h"

int main(int argc, char *argv[])
{
	memory_init();

	char* filename = argv[1];
	if (load_cartridge(filename) != 0)
	{
		printf("File I/O Error\n");
		return 1;
	}

	video_init();

	reset();

	SDL_Event event;
	bool quit = false;

	while (!quit) {

		const uint8_t* keys = SDL_GetKeyboardState(NULL);

		while ( SDL_PollEvent( &event ) )
		{
			reset_controller();

			if (keys[SDL_SCANCODE_ESCAPE])
				quit = true;

			if (keys[SDL_SCANCODE_SLASH])
				controller_state |= BUTTON_A;
			if (keys[SDL_SCANCODE_PERIOD])
				controller_state |= BUTTON_B;
			if (keys[SDL_SCANCODE_RSHIFT])
				controller_state |= BUTTON_SELECT;
			if (keys[SDL_SCANCODE_RETURN])
				controller_state |= BUTTON_START;
			if (keys[SDL_SCANCODE_UP])
				controller_state |= BUTTON_UP;
			if (keys[SDL_SCANCODE_DOWN])
				controller_state |= BUTTON_DOWN;
			if (keys[SDL_SCANCODE_LEFT])
				controller_state |= BUTTON_LEFT;
			if (keys[SDL_SCANCODE_RIGHT])
				controller_state |= BUTTON_RIGHT;
		}

		clock();
	}

	SDL_Quit();
	exit(0);
}
