/* $Id$ */

#include <kos.h>

#include "gen-emu.h"
#include "cart.h"

#include "z80.h"
#include "m68k.h"


UINT8 mz80_read_memory(UINT32 addr, struct MemoryReadByte *mrb);
void mz80_write_memory(UINT32 addr, UINT8 val, struct MemoryWriteByte *mwr);

static struct z80PortRead ReadPorts[] = { { 0xffff, 0xffff, NULL } };
static struct z80PortWrite WritePorts[] = { { 0xffff, 0xffff, NULL } };
static struct MemoryReadByte ReadMemory[] = {
	{ 0x0000, 0xffff, mz80_read_memory },
	{ 0xffffffff, 0xffffffff, NULL }
};
static struct MemoryWriteByte WriteMemory[] = {
	{ 0x0000, 0xffff, mz80_write_memory },
	{ 0xffffffff, 0xffffffff -1, NULL }
};

uint8_t mz80_ram[8192];

uint32_t z80_bank_base;
uint8_t z80_bank_shift;
uint8_t z80_running;
uint8_t z80_busreq;

static struct mz80context z80cpu;


uint32_t z80_init(void)
{
	z80cpu.z80Base = mz80_ram;
	z80cpu.z80MemRead = ReadMemory;
	z80cpu.z80MemWrite = WriteMemory;
	z80cpu.z80IoRead = ReadPorts;
	z80cpu.z80IoWrite = WritePorts;

	mz80SetContext(&z80cpu);

	mz80reset();

	return 0;
}

UINT8 mz80_read_memory(UINT32 addr, struct MemoryReadByte *mrb)
{
	UINT8 ret = 0xff;
	addr &= 0xffff;

	if (addr < 0x4000) {
		ret = mz80_ram[addr & 0x1fff];
	} else
	if (addr >= 0x8000) {
		ret = m68k_read_memory_8(addr & 0x7fff);
	} else
	if ((addr >= 0x7f00) && (addr < 0x7f20)) {
		/* vdp */
	}

	printf("MZ80  %04lx -> %02x\n", addr, ret);

	return(ret);
}

void mz80_write_memory(UINT32 addr, UINT8 val, struct MemoryWriteByte *mwr)
{
	addr &= 0xffff;

	printf("MZ80  %04lx <- %02x\n", addr, val);

	if (addr < 0x4000) {
		mz80_ram[addr & 0x1fff] = val;
	} else
	if (addr >= 0x8000) {
		m68k_write_memory_8(addr & 0x7fff, val);
	} else
	if ((addr >= 0x7f00) && (addr <= 0x7f1f)) {
		/* vdp */
	} else
	if ((addr >= 0x6000) && (addr <= 0x60ff)) {
		static uint32_t tmp_bank_base;
		tmp_bank_base |= ((val & 0x01) << z80_bank_shift++);
		if (z80_bank_shift == 9) {
			z80_bank_base = tmp_bank_base << 15;
			z80_bank_shift = 0;
		}
	}
}

uint32_t z80_enabled(void)
{
	return (z80_running && !z80_busreq);
}
