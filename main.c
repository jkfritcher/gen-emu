
#include <kos.h>

#include "gen-emu.h"

extern uint8_t *rom;
extern uint8_t *m68k_ram;
extern uint8_t *mz80_ram;

//extern uint8 romdisk[];


KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

//KOS_INIT_ROMDISK(romdisk);

//char *romname = "/pc/home/jkf/src/dc/gen-emu/roms/Sonic.bin";
char *romname = "/cd/Sonic.smd";

uint8_t debug = 1;


uint8_t *rom_load(char *name);


int main(int argc, char *argv[])
{
	rom = rom_load(romname);

	free(rom);

	return 0;
}
