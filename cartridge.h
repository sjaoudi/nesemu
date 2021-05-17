#include <stdint.h>

#define FLAG_6_MIRRORING (1 << 0)

enum 			mirroring_mode { Horizontal, Vertical };
extern enum 		mirroring_mode cartridge_mirroring;

int 	load_cartridge(char* filename);

