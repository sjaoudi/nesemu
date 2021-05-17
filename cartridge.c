#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cartridge.h"
#include "system.h"
#include "memory.h"

struct INES_Header
{
	char id[4];
	uint8_t n_prg_banks;
	uint8_t n_chr_banks;
	uint8_t flags6;
	uint8_t flags7;
	uint8_t flags8;
	uint8_t flags9;
	uint8_t flags10;
	char unused[5];
} header;

enum mirroring_mode cartridge_mirroring;

int load_cartridge(char* filename)
{
	FILE* stream = fopen(filename, "rb");

	if (stream != NULL)
	{
		struct INES_Header header;
		fread(&header, sizeof(struct INES_Header), 1, stream);

		fread(cpu_memory + 0xC000 - (header.n_prg_banks - 1) * 0x4000, sizeof(uint8_t), 0x4000 * header.n_prg_banks, stream);

		fread(ppu_memory, sizeof(uint8_t), 0x2000 * header.n_chr_banks, stream);
		
		cartridge_mirroring = (header.flags6 & FLAG_6_MIRRORING) ? Vertical : Horizontal;

		return 0;
	}

	return 1;
}

