#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "video.h"
#include "ppu.h"

graphics_t graphics;
uint8_t *screen;

void video_init()
{
	screen = malloc(WIDTH * HEIGHT * CHANNELS);
	SDL_CreateWindowAndRenderer(WIDTH * SCALE, HEIGHT * SCALE, 0, &graphics.window, &graphics.renderer);
	graphics.texture = SDL_CreateTexture(graphics.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	SDL_SetWindowSize(graphics.window, WIDTH * SCALE, HEIGHT * SCALE);
	SDL_SetWindowTitle(graphics.window, "NES");
}

void video_display_frame()
{
	SDL_UpdateTexture(graphics.texture, NULL, screen, WIDTH * CHANNELS);

	SDL_RenderClear(graphics.renderer);
	SDL_RenderCopy(graphics.renderer, graphics.texture, NULL, NULL);
	SDL_RenderPresent(graphics.renderer);

	memset(screen, 0, WIDTH * HEIGHT * CHANNELS);
}
