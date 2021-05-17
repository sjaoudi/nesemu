#include <SDL2/SDL.h>

#define CHANNELS 3
#define HEIGHT 240 
#define WIDTH 256
#define SCALE 4

void video_init();
void video_display_frame();

typedef struct graphics_t
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
} graphics_t;

extern uint8_t *screen;
