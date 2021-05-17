#include "cpu.h"
#include "controller.h"
#include "memory.h"
#include "system.h"

uint16_t 	pc;
uint8_t 	cycles;
uint32_t 	counter;
bool 		page_crossed;

void cpu_reset()
{
	cpu_registers.a = 0x00;
	cpu_registers.x = 0x00;
	cpu_registers.y = 0x00;
	cpu_registers.sp = 0xFD;
	cpu_registers.p = 0x00;

	set_cpu_flag(FLAG_U, true);
	set_cpu_flag(FLAG_I, true);

	uint8_t lo = cpu_read(RESET_VECTOR);
	uint8_t hi = cpu_read(RESET_VECTOR + 1);

	pc = (hi << 8) | lo;
	cycles = 8;

	counter = 0;
}

// Addressing modes
static inline uint16_t absolute()
{
	uint16_t lo = cpu_read(pc);
	pc++;

	uint16_t hi = cpu_read(pc);
	pc++;

	uint16_t address = (hi << 8) | lo;
	return address;
}

static inline uint16_t immediate()
{
	uint16_t address = pc++;
	return address;
}

static inline uint16_t zeropage()
{
	uint16_t address = cpu_read(pc);
	address &= 0x00FF;
	pc++;

	return address;
}

static inline uint16_t zeropagex()
{
	uint16_t address = (cpu_read(pc) + cpu_registers.x);
	address &= 0x00FF;
	pc++;

	return address;
}

static inline uint16_t zeropagey()
{
	uint16_t address = (cpu_read(pc) + cpu_registers.y);
	address &= 0x00FF;
	pc++;

	return address;
}

static inline uint16_t absolutex()
{
	uint16_t lo = cpu_read(pc);
	pc++;
	uint16_t hi = cpu_read(pc);
	pc++;

	uint16_t address = (hi << 8) | lo;
	address += cpu_registers.x;

	if ((address & 0xFF00) != (hi << 8))
		page_crossed = true;


	return address;
}

static inline uint16_t absolutey()
{
	uint16_t lo = cpu_read(pc);
	pc++;
	uint16_t hi = cpu_read(pc);
	pc++;

	uint16_t address = (hi << 8) | lo;
	address += cpu_registers.y;

	if ((address & 0xFF00) != (hi << 8))
		page_crossed = true;

	return address;
}

static inline uint16_t indirect()
{
	uint16_t lo = cpu_read(pc);
	pc++;
	uint16_t hi = cpu_read(pc);
	pc++;

	uint16_t ptr = (hi << 8) | lo;
	uint16_t address;

	if (lo == 0x00FF)
		address = (cpu_read(ptr & 0xFF00) << 8) | cpu_read(ptr);
	else
		address = (cpu_read(ptr + 1) << 8 | cpu_read(ptr));

	return address;
}

static inline uint16_t indirectx()
{
	uint16_t m = cpu_read(pc);
	pc++;

	uint16_t lo = cpu_read((m + cpu_registers.x) & 0x00FF);
	uint16_t hi = cpu_read((m + cpu_registers.x + 1) & 0x00FF);

	uint16_t address = (hi << 8) | lo;

	return address;
}

static inline uint16_t indirecty()
{
	uint16_t m = cpu_read(pc);
	pc++;

	uint16_t lo = cpu_read(m & 0x00FF);
	uint16_t hi = cpu_read((m + 1) & 0x00FF);

	uint16_t address = (hi << 8) | lo;
	address += cpu_registers.y;

	if ((address & 0xFF00) != (hi << 8))
		page_crossed = true;

	return address;
}

static inline uint16_t relative()
{
	uint16_t address = cpu_read(pc);
	pc++;

	if (address & 0x80)
		address |= 0xFF00;

	return address;
}

// Logical & arithmetic commands
static inline void ora(uint16_t address)
{
	uint8_t m = cpu_read(address);
	cpu_registers.a |= m;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void and(uint16_t address)
{
	uint8_t m = cpu_read(address);
	cpu_registers.a &= m;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void eor(uint16_t address)
{
	uint8_t m = cpu_read(address);
	cpu_registers.a ^= m;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void adc(uint16_t address) 
{
	uint16_t m = cpu_read(address);
	uint16_t sum = cpu_registers.a + m + (is_cpu_flag_set(FLAG_C) ? 1 : 0);

	set_cpu_flag(FLAG_C, sum > 0x00FF);
	set_cpu_flag(FLAG_Z, (sum & 0x00FF) == 0x0000);
	set_cpu_flag(FLAG_N, sum & 0x0080);
	set_cpu_flag(FLAG_V, (~(cpu_registers.a ^ m) & (cpu_registers.a ^ sum)) & 0x0080);

	cpu_registers.a = sum & 0xFF;
}

static inline void sbc(uint16_t address)
{
	uint16_t m = cpu_read(address);
	m ^= 0x00FF;
	uint16_t sum = cpu_registers.a + m + (is_cpu_flag_set(FLAG_C) ? 1 : 0);

	set_cpu_flag(FLAG_C, sum & 0xFF00);
	set_cpu_flag(FLAG_Z, (sum & 0x00FF) == 0x0000);
	set_cpu_flag(FLAG_N, sum & 0x0080);
	set_cpu_flag(FLAG_V, (sum ^ cpu_registers.a) & (sum ^ m) & 0x0080);

	cpu_registers.a = sum & 0xFF;
}

static inline void cmp(uint16_t address)
{
	uint8_t m = cpu_read(address);
	set_cpu_flag(FLAG_Z, cpu_registers.a == m);
	set_cpu_flag(FLAG_C, cpu_registers.a >= m);
	set_cpu_flag(FLAG_N, (cpu_registers.a - m) & 0x80);
}

static inline void cpx(uint16_t address)
{
	uint8_t m = cpu_read(address);
	set_cpu_flag(FLAG_Z, cpu_registers.x == m);
	set_cpu_flag(FLAG_C, cpu_registers.x >= m);
	set_cpu_flag(FLAG_N, (cpu_registers.x - m) & 0x80);
}

static inline void cpy(uint16_t address)
{
	uint8_t m = cpu_read(address);
	set_cpu_flag(FLAG_Z, cpu_registers.y == m);
	set_cpu_flag(FLAG_C, cpu_registers.y >= m);
	set_cpu_flag(FLAG_N, (cpu_registers.y - m) & 0x80);
}

static inline void dec(uint16_t address)
{
	uint8_t m = cpu_read(address);
	m--;

	cpu_write(address, m);

	set_cpu_flag(FLAG_Z, m == 0x00);
	set_cpu_flag(FLAG_N, m & 0x80);
}

static inline void dex()
{
	cpu_registers.x--;

	set_cpu_flag(FLAG_Z, cpu_registers.x == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.x & 0x80);
}

static inline void dey()
{
	cpu_registers.y--;

	set_cpu_flag(FLAG_Z, cpu_registers.y == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.y & 0x80);
}

static inline void inc(uint16_t address)
{
	uint8_t m = cpu_read(address);
	m++;

	cpu_write(address, m);

	set_cpu_flag(FLAG_Z, m == 0x00);
	set_cpu_flag(FLAG_N, m & 0x80);
}

static inline void inx()
{
	cpu_registers.x++;

	set_cpu_flag(FLAG_Z, cpu_registers.x == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.x & 0x80);
}

static inline void iny()
{
	cpu_registers.y++;

	set_cpu_flag(FLAG_Z, cpu_registers.y == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.y & 0x80);
}

static inline void asl_a()
{
	set_cpu_flag(FLAG_C, cpu_registers.a & 0x80);

	cpu_registers.a <<= 1;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void asl_m(uint16_t address)
{
	uint8_t m = cpu_read(address);
	set_cpu_flag(FLAG_C, m & 0x80);

	m <<= 1;

	set_cpu_flag(FLAG_Z, m == 0x00);
	set_cpu_flag(FLAG_N, m & 0x80);
	cpu_write(address, m);
}

static inline void rol_a()
{
	uint8_t a_prev = cpu_registers.a;
	cpu_registers.a <<= 1;

	if (is_cpu_flag_set(FLAG_C))
		cpu_registers.a |= 0x01;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_C, a_prev & 0x80);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void rol_m(uint16_t address)
{
	uint8_t m = cpu_read(address);
	uint8_t m_prev = m;
	m <<= 1;

	if (is_cpu_flag_set(FLAG_C))
		m |= 0x01;

	set_cpu_flag(FLAG_Z, m == 0x00);
	set_cpu_flag(FLAG_C, m_prev & 0x80);
	set_cpu_flag(FLAG_N, m & 0x80);

	cpu_write(address, m);
}

static inline void lsr_a()
{
	set_cpu_flag(FLAG_C, cpu_registers.a & 0x01);

	cpu_registers.a >>= 1;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void lsr_m(uint16_t address)
{
	uint8_t m = cpu_read(address);

	set_cpu_flag(FLAG_C, m & 0x01);

	m >>= 1;

	set_cpu_flag(FLAG_Z, m == 0x00);
	set_cpu_flag(FLAG_N, m & 0x80);

	cpu_write(address, m);
}

static inline void ror_a()
{
	uint8_t a_prev = cpu_registers.a;
	cpu_registers.a >>= 1;

	if (is_cpu_flag_set(FLAG_C))
		cpu_registers.a |= 0x80;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
	set_cpu_flag(FLAG_C, a_prev & 0x01);
}

static inline void ror_m(uint16_t address)
{
	uint8_t m = cpu_read(address);
	uint8_t m_prev = m;
	m >>= 1;

	if (is_cpu_flag_set(FLAG_C))
		m |= 0x80;

	set_cpu_flag(FLAG_Z, m == 0x00);
	set_cpu_flag(FLAG_N, m & 0x80);
	set_cpu_flag(FLAG_C, m_prev & 0x01);

	cpu_write(address, m);
}


// Move commands

static inline void lda(uint16_t address)
{
	uint8_t m = cpu_read(address);
	cpu_registers.a = m;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void sta(uint16_t address)
{
	cpu_write(address, cpu_registers.a);
}

static inline void ldx(uint16_t address)
{
	uint8_t m = cpu_read(address);
	cpu_registers.x = m;

	set_cpu_flag(FLAG_Z, cpu_registers.x == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.x & 0x80);
}

static inline void stx(uint16_t address)
{
	cpu_write(address, cpu_registers.x);
}

static inline void ldy(uint16_t address)
{
	uint8_t m = cpu_read(address);
	cpu_registers.y = m;

	set_cpu_flag(FLAG_Z, cpu_registers.y == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.y & 0x80);
}

static inline void sty(uint16_t address)
{
	cpu_write(address, cpu_registers.y);
}

static inline void tax()
{
	cpu_registers.x = cpu_registers.a;
	set_cpu_flag(FLAG_Z, cpu_registers.x == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.x & 0x80);
}

static inline void txa()
{
	cpu_registers.a = cpu_registers.x;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void tay()
{
	cpu_registers.y = cpu_registers.a;

	set_cpu_flag(FLAG_Z, cpu_registers.y == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.y & 0x80);
}

static inline void tya()
{
	cpu_registers.a = cpu_registers.y;

	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void tsx()
{
	cpu_registers.x = cpu_registers.sp;

	set_cpu_flag(FLAG_Z, cpu_registers.x == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.x & 0x80);
}

static inline void txs()
{
	cpu_registers.sp = cpu_registers.x;
}

static inline void pla()
{
	cpu_registers.sp++;
	cpu_registers.a = cpu_read(0x100 + cpu_registers.sp);
	set_cpu_flag(FLAG_Z, cpu_registers.a == 0x00);
	set_cpu_flag(FLAG_N, cpu_registers.a & 0x80);
}

static inline void pha()
{
	cpu_write(0x0100 + cpu_registers.sp, cpu_registers.a);
	cpu_registers.sp--;
}

static inline void plp()
{
	cpu_registers.sp++;
	cpu_registers.p = cpu_read(0x100 + cpu_registers.sp);
}

static inline void php()
{
	set_cpu_flag(FLAG_U, true);
	set_cpu_flag(FLAG_B, true);
	cpu_write(0x0100 + cpu_registers.sp, cpu_registers.p);
	cpu_registers.sp--;
}


// Jump commands

static inline void _branch(uint16_t address)
{
	address += pc;

	if ((address & 0xFF00) != (pc & 0xFF00))
		page_crossed = true;

	pc = address;
}

static inline void bpl(uint16_t address)
{
	if (!is_cpu_flag_set(FLAG_N))
		_branch(address);
}

static inline void bmi(uint16_t address)
{
	if (is_cpu_flag_set(FLAG_N))
		_branch(address);
}

static inline void bvc(uint16_t address)
{
	if (!is_cpu_flag_set(FLAG_V))
		_branch(address);
}

static inline void bvs(uint16_t address)
{
	if (is_cpu_flag_set(FLAG_V))
		_branch(address);
}

static inline void bcc(uint16_t address)
{
	if (!is_cpu_flag_set(FLAG_C))
		_branch(address);
}

static inline void bcs(uint16_t address)
{
	if (is_cpu_flag_set(FLAG_C))
		_branch(address);
}

static inline void bne(uint16_t address)
{
	if (!is_cpu_flag_set(FLAG_Z))
		_branch(address);
}

static inline void beq(uint16_t address)
{
	if (is_cpu_flag_set(FLAG_Z))
		_branch(address);
}

static inline void brk()
{
	pc++;

	cpu_write(0x0100 + cpu_registers.sp, (pc >> 8) & 0x00FF);
	cpu_registers.sp--;
	cpu_write(0x0100 + cpu_registers.sp, pc & 0x00FF);
	cpu_registers.sp--;

	set_cpu_flag(FLAG_U, true);
	set_cpu_flag(FLAG_B, true);
	cpu_write(0x0100 + cpu_registers.sp, cpu_registers.p);
	cpu_registers.sp--;
	
	set_cpu_flag(FLAG_I, true);

	uint16_t lo = cpu_read(IRQ_VECTOR);
	uint16_t hi = cpu_read(IRQ_VECTOR + 1);

	pc = (hi << 8) | lo;
}

static inline void rti()
{
	cpu_registers.sp++;
	cpu_registers.p = cpu_read(0x0100 + cpu_registers.sp);
	set_cpu_flag(FLAG_B, false);
	set_cpu_flag(FLAG_U, false);

	cpu_registers.sp++;
	uint8_t lo = cpu_read(0x100 + cpu_registers.sp);
	cpu_registers.sp++;
	uint8_t hi = cpu_read(0x100 + cpu_registers.sp);

	pc = (hi << 8) | lo;
}

void nmi()
{
	cpu_write(0x0100 + cpu_registers.sp, pc >> 8);
	cpu_registers.sp--;
	cpu_write(0x0100 + cpu_registers.sp, pc);
	cpu_registers.sp--;

	set_cpu_flag(FLAG_B, false);
	set_cpu_flag(FLAG_U, true);
	set_cpu_flag(FLAG_I, true);
	cpu_write(0x0100 + cpu_registers.sp, cpu_registers.p);
	cpu_registers.sp--;

	uint8_t lo = cpu_read(NMI_VECTOR);
	uint8_t hi = cpu_read(NMI_VECTOR + 1);

	pc = (hi << 8) | lo;

	cycles = 8;
}

static inline void jsr(uint16_t address)
{
	pc--;
	cpu_write(0x0100 + cpu_registers.sp, (pc >> 8) & 0x00FF);
	cpu_registers.sp--;
	cpu_write(0x0100 + cpu_registers.sp, pc & 0x00FF);
	cpu_registers.sp--;

	pc = address;
}

static inline void rts()
{
	cpu_registers.sp++;
	uint8_t lo = cpu_read(0x100 + cpu_registers.sp);
	cpu_registers.sp++;
	uint8_t hi = cpu_read(0x100 + cpu_registers.sp);

	pc = (hi << 8) | lo;
	pc++;
}

static inline void jmp(uint16_t address)
{
	pc = address;
}

static inline void bit(uint16_t address)
{
	uint8_t m = cpu_read(address);

	set_cpu_flag(FLAG_Z, (cpu_registers.a & m) == 0x00);
	set_cpu_flag(FLAG_N, m & 0x80);
	set_cpu_flag(FLAG_V, m & 0x40);
}

static inline void clc()
{
	set_cpu_flag(FLAG_C, false);
}

static inline void sec()
{
	set_cpu_flag(FLAG_C, true);
}

static inline void cld()
{
	set_cpu_flag(FLAG_D, false);
}

static inline void sed()
{
	set_cpu_flag(FLAG_D, true);
}

static inline void cli()
{
	set_cpu_flag(FLAG_I, false);
}

static inline void sei()
{
	set_cpu_flag(FLAG_I, true);
}

static inline void clv()
{
	set_cpu_flag(FLAG_V, false);
}

// Illegal opcodes
static inline void nop() { };

static inline void xxx() 
{
}

void cpu_clock()
{
	if (cycles == 0) 
	{
		uint8_t opcode = cpu_read(pc);
		//debug();
		pc++;
		
		switch (opcode)
		{
			case 0x69: adc(immediate());	break;
			case 0x65: adc(zeropage());  	break;
			case 0x75: adc(zeropagex());	break;
			case 0x6d: adc(absolute());	break;
			case 0x7d: adc(absolutex());	break;
			case 0x79: adc(absolutey());	break;
			case 0x61: adc(indirectx());	break;
			case 0x71: adc(indirecty());	break;

			case 0x29: and(immediate());	break;
			case 0x25: and(zeropage());	break;
			case 0x35: and(zeropagex());	break;
			case 0x2d: and(absolute());	break;
			case 0x3d: and(absolutex());	break;
			case 0x39: and(absolutey());	break;
			case 0x21: and(indirectx());	break;
			case 0x31: and(indirecty());	break;

			case 0x0a: asl_a();		break;
			case 0x06: asl_m(zeropage());	break;
			case 0x16: asl_m(zeropagex());	break;
			case 0x0e: asl_m(absolute());	break;
			case 0x1e: asl_m(absolutex());	break;

			case 0x90: bcc(relative());	break;
			case 0xb0: bcs(relative());	break;
			case 0xf0: beq(relative());	break;
			case 0x30: bmi(relative());	break;
			case 0xd0: bne(relative());	break;
			case 0x10: bpl(relative());	break;

			case 0x24: bit(zeropage());	break;
			case 0x2c: bit(absolute());	break;

			case 0x00: brk();		break;

			case 0x50: bvc(relative());	break;
			case 0x70: bvs(relative());	break;

			case 0x18: clc();		break;
			case 0xd8: cld();		break;
			case 0x58: cli();		break;
			case 0xb8: clv();		break;

			case 0xc9: cmp(immediate());	break;
			case 0xc5: cmp(zeropage());	break;
			case 0xd5: cmp(zeropagex());	break;
			case 0xcd: cmp(absolute());	break;
			case 0xdd: cmp(absolutex());	break;
			case 0xd9: cmp(absolutey());	break;
			case 0xc1: cmp(indirectx());	break;
			case 0xd1: cmp(indirecty());	break;

			case 0xe0: cpx(immediate());	break;
			case 0xe4: cpx(zeropage());	break;
			case 0xec: cpx(absolute());	break;

			case 0xc0: cpy(immediate());	break;
			case 0xc4: cpy(zeropage());	break;
			case 0xcc: cpy(absolute());	break;

			case 0xc6: dec(zeropage());	break;
			case 0xd6: dec(zeropagex());	break;
			case 0xce: dec(absolute());	break;
			case 0xde: dec(absolutex());	break;

			case 0xca: dex();		break;
			case 0x88: dey();		break;

			case 0x49: eor(immediate());	break;
			case 0x45: eor(zeropage());	break;
			case 0x55: eor(zeropagex());	break;
			case 0x4d: eor(absolute());	break;
			case 0x5d: eor(absolutex());	break;
			case 0x59: eor(absolutey());	break;
			case 0x41: eor(indirectx());	break;
			case 0x51: eor(indirecty());	break;

			case 0xe6: inc(zeropage());	break;
			case 0xf6: inc(zeropagex());	break;
			case 0xee: inc(absolute());	break;
			case 0xfe: inc(absolutex());	break;

			case 0xe8: inx();		break;
			case 0xc8: iny();		break;

			case 0x4c: jmp(absolute());	break;
			case 0x6c: jmp(indirect());	break;

			case 0x20: jsr(absolute());	break;

			case 0xa9: lda(immediate());	break;
			case 0xa5: lda(zeropage());	break;
			case 0xb5: lda(zeropagex());	break;
			case 0xad: lda(absolute());	break;
			case 0xbd: lda(absolutex());	break;
			case 0xb9: lda(absolutey());	break;
			case 0xa1: lda(indirectx());	break;
			case 0xb1: lda(indirecty());	break;

			case 0xa2: ldx(immediate());	break;
			case 0xa6: ldx(zeropage());	break;
			case 0xb6: ldx(zeropagey());	break;
			case 0xae: ldx(absolute());	break;
			case 0xbe: ldx(absolutey());	break;

			case 0xa0: ldy(immediate());	break;
			case 0xa4: ldy(zeropage());	break;
			case 0xb4: ldy(zeropagex());	break;
			case 0xac: ldy(absolute());	break;
			case 0xbc: ldy(absolutex());	break;

			case 0x4a: lsr_a();		break;
			case 0x46: lsr_m(zeropage());	break;
			case 0x56: lsr_m(zeropagex());	break;
			case 0x4e: lsr_m(absolute());	break;
			case 0x5e: lsr_m(absolutex());	break;

			case 0x80: nop(immediate());	break;
			case 0x04:
			case 0x44: 
			case 0x64: nop(zeropage());	break;
			case 0x0c: nop(absolute());	break;
			case 0x14: 
			case 0x34: 
			case 0x54: 
			case 0x74: 
			case 0xd4: 
			case 0xf4: nop(zeropagex());	break;
			case 0x1a: 
			case 0x3a: 
			case 0x5a: 
			case 0x7a: 
			case 0xda: 
			case 0xea: 
			case 0xfa: nop();		break;

			case 0x1c: 
			case 0x3c: 
			case 0x5c: 
			case 0x7c: 
			case 0xdc: 
			case 0xfc: nop(absolutex());	break;

			case 0x09: ora(immediate());	break;
			case 0x05: ora(zeropage());	break;
			case 0x15: ora(zeropagex());	break;
			case 0x0d: ora(absolute());	break;
			case 0x1d: ora(absolutex());	break;
			case 0x19: ora(absolutey());	break;
			case 0x01: ora(indirectx());	break;
			case 0x11: ora(indirecty());	break;

			case 0x48: pha(); 		break;
			case 0x08: php(); 		break;
			case 0x68: pla(); 		break;
			case 0x28: plp(); 		break;

			case 0x2a: rol_a();		break;
			case 0x26: rol_m(zeropage());	break;
			case 0x36: rol_m(zeropagex());	break;
			case 0x2e: rol_m(absolute());	break;
			case 0x3e: rol_m(absolutex());	break;

			case 0x6a: ror_a();		break;
			case 0x66: ror_m(zeropage());	break;
			case 0x76: ror_m(zeropagex());	break;
			case 0x6e: ror_m(absolute());	break;
			case 0x7e: ror_m(absolutex());	break;

			case 0x40: rti(); 		break;

			case 0x60: rts(); 		break;

			case 0xe9: sbc(immediate()); 	break;
			case 0xe5: sbc(zeropage()); 	break;
			case 0xf5: sbc(zeropagex()); 	break;
			case 0xed: sbc(absolute()); 	break;
			case 0xfd: sbc(absolutex()); 	break;
			case 0xf9: sbc(absolutey()); 	break;
			case 0xe1: sbc(indirectx()); 	break;
			case 0xf1: sbc(indirecty()); 	break;

			case 0x38: sec(); 		break;
			case 0xf8: sed(); 		break;

			case 0x78: sei(); 		break;

			case 0x85: sta(zeropage()); 	break;
			case 0x95: sta(zeropagex()); 	break;
			case 0x8d: sta(absolute()); 	break;
			case 0x9d: sta(absolutex()); 	break;
			case 0x99: sta(absolutey()); 	break;
			case 0x81: sta(indirectx()); 	break;
			case 0x91: sta(indirecty()); 	break;

			case 0x86: stx(zeropage());	break;
			case 0x96: stx(zeropagey());	break;
			case 0x8e: stx(absolute());	break;

			case 0x84: sty(zeropage()); 	break;
			case 0x94: sty(zeropagex());	break;
			case 0x8c: sty(absolute());	break;

			case 0xaa: tax(); 		break;
			case 0xa8: tay(); 		break;
			case 0xba: tsx(); 		break;
			case 0x8a: txa(); 		break;
			case 0x9a: txs(); 		break;
			case 0x98: tya(); 		break;

			default: 
				printf("ILLEGAL OPCODE: %X\n", opcode);
				exit(1);
				break;
		}

		cycles += lut_cycles[opcode];

		if (page_crossed)
		{
			if (lut_pagecrosses[opcode])
			{
				cycles++;
			}
			page_crossed = false;
		}

		counter += cycles;
	}

	cycles--;
}

