#include "memory.h"
#include "cartridge.h"
#include "ppu.h"
#include "cpu.h"
#include "controller.h"

uint8_t 	*ppu_memory;
uint8_t 	*cpu_memory;
uint8_t 	*primary_oam;

uint16_t 	ppu_read_buffer;

struct 		CPU_Registers cpu_registers;
struct 		PPU_Registers ppu_registers;

void memory_init()
{
	ppu_memory = malloc(0x4000);
	if (ppu_memory != NULL)
		memset(ppu_memory, 0, 0x4000);

	cpu_memory = malloc(0xFFFF);
	if (cpu_memory != NULL)
		memset(cpu_memory, 0, 0xFFFF);

	primary_oam = malloc(0xFF);
	if (primary_oam != NULL)
		memset(primary_oam, 0xFF, 0xFF);

	ppu_read_buffer = 0x0000;
}

uint8_t ppu_read(uint16_t address)
{
	uint8_t data = 0x00;

	if (address <= 0x1FFF)
	{
		data = ppu_memory[address];
	}
	else if (address >= 0x2000 && address <= 0x3EFF)
	{
		if (cartridge_mirroring == Horizontal)
		{
			if (address >= 0x3000)
				address -= 0x1000;

			if (address >= 0x2000 && address <= 0x23FF)
				data = ppu_memory[address];
			else if (address >= 0x2400 && address <= 0x27FF)
				data = ppu_memory[address];
			else if (address >= 0x2800 && address <= 0x2BFF)
				data = ppu_memory[address + 0x0400];
			else if (address >= 0x2C00 && address <= 0x2FFF)
				data = ppu_memory[address + 0x0400];
		}
		else if (cartridge_mirroring == Vertical)
		{
			if (address >= 0x2000 && address <= 0x23FF)
				data = ppu_memory[address];
			else if (address >= 0x2400 && address <= 0x27FF)
				data = ppu_memory[address + 0x0400];
			else if (address >= 0x2800 && address <= 0x2BFF)
				data = ppu_memory[address];
			else if (address >= 0x2C00 && address <= 0x2FFF)
				data = ppu_memory[address + 0x0400];
		}
	}
	else if (address >= 0x3F00 && address <= 0x3FFF)
	{
		if (address == 0x3F10)
			address = 0x3F00;
		if (address == 0x3F14)
			address = 0x3F04;
		if (address == 0x3F18)
			address = 0x3F08;
		if (address == 0x3F1C)
			address = 0x3F0C;

		data = ppu_memory[address];
	}

	return data;
}

void ppu_write(uint16_t address, uint8_t data)
{
	if (address <= 0x1FFF)
	{
		ppu_memory[address] = data;
	}
	else if (address >= 0x2000 && address <= 0x3EFF)
	{
		if (address >= 0x3000)
			address -= 0x1000;

		if (cartridge_mirroring == Horizontal)
		{
			if (address >= 0x2000 && address <= 0x23FF)
				ppu_memory[address] = data;
			else if (address >= 0x2400 && address <= 0x27FF)
				ppu_memory[address] = data;
			else if (address >= 0x2800 && address <= 0x2BFF)
				ppu_memory[address + 0x0400] = data;
			else if (address >= 0x2C00 && address <= 0x2FFF)
				ppu_memory[address + 0x0400] = data;
		}
		else if (cartridge_mirroring == Vertical)
		{
			if (address >= 0x2000 && address <= 0x23FF)
				ppu_memory[address] = data;
			else if (address >= 0x2400 && address <= 0x27FF)
				ppu_memory[address + 0x0400] = data;
			else if (address >= 0x2800 && address <= 0x2BFF)
				ppu_memory[address] = data;
			else if (address >= 0x2C00 && address <= 0x2FFF)
				ppu_memory[address + 0x0400] = data;
		}
	}
	else if (address >= 0x3F00 && address <= 0x3FFF)
	{
		if (address == 0x3F10)
			address = 0x3F00;
		if (address == 0x3F14)
			address = 0x3F04;
		if (address == 0x3F18)
			address = 0x3F08;
		if (address == 0x3F1C)
			address = 0x3F0C;

		ppu_memory[address] = data;
	}
}

void set_ppu_flag(uint16_t reg, uint8_t flag, bool condition)
{
	uint8_t r = cpu_memory[reg];

	if (condition)
		r |= flag;
	else
		r &= ~flag;

	cpu_memory[reg] = r;
}

bool is_ppu_flag_set(uint16_t reg, uint8_t flag)
{
	uint8_t r = cpu_memory[reg];
	return r & flag;
}

uint8_t cpu_read(uint16_t address)
{
	uint8_t data = 0x00;

	if (address >= 0x8000 && address <= 0xFFFF)
	{
		data = cpu_memory[address];
	}
	else if (address <= 0x1FFF)
	{
		data = cpu_memory[address];
	}
	else if (address >= 0x2000 && address <= 0x3FFF)
	{
		address &= 0x2007;

		switch (address)
		{
			case (0x2000): // control
				break;
			case (0x2001): // mask
				break;
			case (0x2002): // status
				data = (cpu_memory[0x2002] & 0xE0) | (ppu_read_buffer & 0x1F);

				ppu_registers.w = 0;
				set_ppu_flag(PPUSTATUS, PPUSTATUS_FLAG_V, false);

				break;
			case (0x2006): // address
				break;
			case (0x2007): // data
				if (ppu_registers.v <= 0x3EFF)
				{
					data = ppu_read_buffer;
					ppu_read_buffer = ppu_read(ppu_registers.v);
				}
				else if (ppu_registers.v >= 0x3F00 && ppu_registers.v <= 0x3FFF)
					data = ppu_read(ppu_registers.v);

				ppu_registers.v += (is_ppu_flag_set(PPUCTRL, PPUCTRL_FLAG_I) ? 32 : 1);

				break;
		}
	}
	else if (address == 0x4016)
	{
		data = read_controller();
	}

	return data;
}

void cpu_write(uint16_t address, uint8_t data)
{
	// cartridge mapping
	if (address >= 0x8000 && address <= 0xFFFF)
	{
		cpu_memory[address] = data;
	}
	else if (address <= 0x1FFF)
	{
		cpu_memory[address] = data;
	}
	else if (address >= 0x2000 && address <= 0x3FFF)
	{
		address &= 0x2007;

		// write
		switch (address)
		{
			case (0x2000): // control
				ppu_registers.t &= ~0x0C00;
				ppu_registers.t |= (((uint16_t)data & 0x3) << 10);

				cpu_memory[PPUCTRL] = data;

				break;
			case (0x2001): // mask
				cpu_memory[PPUMASK] = data;
				break;
			case (0x2002): // status
				cpu_memory[PPUSTATUS] = (cpu_memory[PPUSTATUS] & 0x80) | (data & 0x3F);
				
				break;
			case (0x2003):
				cpu_memory[OAMADDR] = data;
				break;
			case (0x2004):
				primary_oam[ cpu_memory[OAMADDR] ] = data;
				cpu_memory[OAMADDR] += 1;
				break;
			case (0x2005):
				if (ppu_registers.w == 0)
				{
					// update fine x
					ppu_registers.x = data & 0x07;

					// update coarse x
					ppu_registers.t = (ppu_registers.t & ~0x001F) | ((uint16_t)data >> 3);
				}
				else
				{
					// update fine y
					ppu_registers.t = (ppu_registers.t & ~0x7000) | (((uint16_t)data & 0x7) << 12);

					// update coarse y
					ppu_registers.t = (ppu_registers.t & ~0x03E0) | (((uint16_t)data >> 3) << 5);
				}

				ppu_registers.w ^= 1;

				break;
			case (0x2006): // address
				if (ppu_registers.w == 0)
				{
					ppu_registers.t = (ppu_registers.t & 0x00FF) | (((uint16_t)data & 0x3F) << 8);

					// set bit 14 to 0
					ppu_registers.t = (ppu_registers.t & ~0x4000);
				}
				else
				{
					ppu_registers.t = (ppu_registers.t & 0xFF00) | (uint16_t)data;
					ppu_registers.v = ppu_registers.t;
				}

				ppu_registers.w ^= 1;
					
				break;
			case (0x2007): // data
				ppu_write(ppu_registers.v, data);

				ppu_registers.v += (is_ppu_flag_set(PPUCTRL, PPUCTRL_FLAG_I) ? 32 : 1);

				break;
		}
	}
	else if (address == 0x4014)
	{
		uint16_t oam_address = ((uint16_t)data << 8) | (cpu_memory[OAMADDR] << 8);
		for (uint16_t i = 0; i <= 255; i++)
		{
			uint8_t data = cpu_read(oam_address + i);
			primary_oam[i] = data;
		}
	}
	else if (address == 0x4016)
	{
		write_controller(data);
	}
}

void set_cpu_flag(uint8_t flag, bool condition)
{
	if (condition)
		cpu_registers.p |= flag;
	else
		cpu_registers.p &= ~flag;
}

bool is_cpu_flag_set(uint8_t flag)
{
	return cpu_registers.p & flag;
}
