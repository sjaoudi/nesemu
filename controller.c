#include "controller.h"
#include "memory.h"

uint8_t controller_state;
uint8_t shift_register;

void reset_controller()
{
	controller_state = 0x00;
}

uint8_t read_controller()
{
	uint8_t data;

	if (cpu_memory[0x4016] & STROBE)
	{
		data = (controller_state & STROBE) | 0x40;
		return data;
	}

	data = (shift_register & STROBE) | 0x40;
	shift_register = (shift_register >> 1) | 0x80;

	return data;
}

void write_controller(uint8_t data)
{
	if ((cpu_memory[0x4016] & STROBE) && !(data & STROBE))
	{
		shift_register = controller_state;
	}

	cpu_memory[0x4016] = data;
}

