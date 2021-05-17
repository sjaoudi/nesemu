#include <stdint.h>

#define PPUCTRL 	0x2000
#define PPUMASK 	0x2001
#define PPUSTATUS 	0x2002
#define OAMADDR	  	0x2003
#define OAMDATA   	0x2004
#define PPUSCROLL 	0x2005
#define PPUADDR 	0x2006
#define PPUDATA 	0x2007

#define PPUCTRL_FLAG_I (1 << 2)
#define PPUCTRL_FLAG_S (1 << 3)
#define PPUCTRL_FLAG_B (1 << 4)
#define PPUCTRL_FLAG_H (1 << 5)
#define PPUCTRL_FLAG_V (1 << 7)

#define PPUMASK_FLAG_B (1 << 3)
#define PPUMASK_FLAG_S (1 << 4)

#define PPUSTATUS_FLAG_O (1 << 5)
#define PPUSTATUS_FLAG_S (1 << 6)
#define PPUSTATUS_FLAG_V (1 << 7)

#define R2(n)	n,	n + 2*64,	n + 1*64,	n + 3*64
#define R4(n) 	R2(n), 	R2(n + 2*16), 	R2(n + 1*16), 	R2(n + 3*16)
#define R6(n) 	R4(n), 	R4(n + 2*4 ), 	R4(n + 1*4 ), 	R4(n + 3*4 )
#define REVERSE_BITS	R6(0),	R6(2),	R6(1),	R6(3)

static const uint8_t	lut_reverse8[256] = { REVERSE_BITS };

static const uint32_t 	palette[64] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400, 
	0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10, 
	0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044, 
	0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8, 
	0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};

struct PPU_Registers
{
	uint16_t	v : 15; // current vram address
	uint16_t	t : 15; // temp vram address
	uint8_t 	x : 3;  // fine x scroll
	uint8_t 	w : 1;  // first or second write toggle
};

struct OAM_Entry
{
	uint8_t		y;
	uint8_t		tile;
	uint8_t		attribute;
	uint8_t		x;
};

void 	ppu_clock();
void 	ppu_reset();
void 	debug();

