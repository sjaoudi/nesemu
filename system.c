#include "system.h"
#include "cpu.h"
#include "ppu.h"

bool trigger_nmi;

void clock()
{
	for (uint8_t i = 3; i--;)
		ppu_clock();

	if (trigger_nmi)
	{
		nmi();
		trigger_nmi = false;
	}
	
	cpu_clock();
}

void reset()
{
	cpu_reset();
	ppu_reset();
}

void debug()
{
	//printf("f:%d		%04X  %02X	A:%02X X:%02X Y:%02X P:%02X  SP:%02X\n", 
	//	frame, pc, opcode, registers.a, registers.x, registers.y, registers.p, registers.sp);
	//printf("%04X  %02X %d\n", pc, opcode, counter);
}

