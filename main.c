
#include <kos.h>

#include "gen-emu.h"

extern uint8_t *rom;
extern uint8_t *m68k_ram;
extern uint8_t *mz80_ram;


KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);


char *romname = "/pc/home/jkf/src/dc/gen-emu/roms/sonic-1.smd";
//char *romname = "/cd/sonic_1.bin";

uint8_t debug = 1;


uint8_t *rom_load(char *name);


int main(int argc, char *argv[])
{
	rom = rom_load(romname);

	free(rom);

	return 0;
}
