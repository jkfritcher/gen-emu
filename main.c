#include <kos.h>

#include "gen-emu.h"

#include "m68k.h"
#include "z80.h"
#include "vdp.h"
#include "SN76489.h"
#include "input.h"

//KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS | INIT_GDB);
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);


//char *romname = "/pc/home/jkf/src/dc/gen-emu/roms/ssf2-u.bin";
char *romname = "/pc/home/jkf/src/dc/gen-emu/roms/sonic-1.smd";
//char *romname = "/cd/sonic_1.bin";

uint8_t debug = 0;
uint8_t quit = 0;


uint32_t rom_load(char *name);
void rom_free(void);
void run_one_field(void);

extern SN76489 PSG; 

int main(int argc, char *argv[])
{
	maple_device_t *caddr;
	cont_state_t   *cont;
	int fd;

	caddr = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	if (caddr == NULL) {
		printf("No controllers found. Aborting...\n");
		arch_abort();
	}

	rom_load(romname);

	z80_init();
	m68k_pulse_reset();

        Reset76489(&PSG,0);
        Sync76489(&PSG,SN76489_SYNC); 	

	ctlr_init();


	do {
		run_one_field();
		cont = maple_dev_status(caddr);
		if (cont->buttons & CONT_A)
			debug = !debug;
	} while (!(cont->buttons & CONT_START) && !quit);

	fd = fs_open("/pc/home/jkf/src/dc/gen-emu/68kram.bin", O_WRONLY | O_TRUNC);
	fs_write(fd, m68k_ram, 65536);
	fs_close(fd);

	fd = fs_open("/pc/home/jkf/src/dc/gen-emu/z80ram.bin", O_WRONLY | O_TRUNC);
	fs_write(fd, z80_ram, 8192);
	fs_close(fd);

	rom_free();

	return 0;
}

void run_one_field(void)
{
	static int cnt;
	int line;


	for(line = 0; line < 262 && !quit; line++) {
		vdp_interrupt(line);

		m68k_execute(488);
		if (z80_enabled())
			z80_execute(228);

		/* render scanline to vram*/
	}

	/* Submit whole screen to pvr. */

	/* sound stuff, call once per frame */
	Sync76489(&PSG,SN76489_FLUSH);

	display_cram();

	/* input processing */
	cnt++;
	if ((cnt % 60) == 0)
		printf(".");
}
