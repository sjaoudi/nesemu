#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void 		memory_init();

uint8_t 	cpu_read(uint16_t address);
void 		cpu_write(uint16_t address, uint8_t data);

uint8_t 	ppu_read(uint16_t address);
void 		ppu_write(uint16_t address, uint8_t data);

void 		set_cpu_flag(uint8_t flag, bool condition);
bool 		is_cpu_flag_set(uint8_t flag);

bool 		is_ppu_flag_set(uint16_t reg, uint8_t flag);
void 		set_ppu_flag(uint16_t reg, uint8_t flag, bool condition);

extern uint8_t*	cpu_memory;
extern uint8_t*	ppu_memory;
extern uint8_t*	primary_oam;

extern struct 	CPU_Registers cpu_registers;
extern struct 	PPU_Registers ppu_registers;
