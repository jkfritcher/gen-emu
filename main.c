#include <kos.h>

#include "gen-emu.h"

#include "m68k.h"
#include "z80.h"
#include "vdp.h"
#include "SN76489.h"

//KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS | INIT_GDB);
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS | INIT_OCRAM);


//char *romname = "/cd/sonic_1.bin";
//char *romname = "/cd/pstar_2.bin";
char *romname = "/cd/contra.bin";

char *scrcapname = "/pc/home/jkf/src/dc/gen-emu/screen.ppm";

uint8_t debug = 0;
uint8_t quit = 0;
uint8_t dump = 0;
uint8_t pause = 0;

uint32_t rom_load(char *name);
void rom_free(void);
void run_one_field(void);
void gen_init(void);
void gen_reset(void);

extern SN76489 PSG; 
extern struct vdp_s vdp;


int main(int argc, char *argv[])
{
	int ch, fd;

	gen_init();

	rom_load(romname);

	gen_reset();

	do {
		run_one_field();

paused:
		ch = kbd_get_key();
		switch(ch) {
		case 0x000d:	/* Enter */
			pause = !pause;
			break;
		case 0x0020:	/* Space */
			quit = 1;
			break;
		case 0x0039:	/* 9 */
			dump = 1;
			break;
		case 0x4600:	/* Print Screen */
			vid_screen_shot(scrcapname);
			fd = fs_open("/pc/home/jkf/src/dc/gen-emu/vdp.bin", O_WRONLY | O_TRUNC);
			fs_write(fd, &vdp, sizeof(vdp));
			fs_close(fd);
			break;
		}
		if (pause)
			goto paused;
	} while (!quit);

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

		if (line < 224) {
			/* render scanline to vram*/
			vdp_render_scanline(line);
		}
	}

	/* Render debug cram display. */
#if 0
	vdp_render_cram();
#endif

	/* Submit whole screen to pvr. */
	do_frame();

	/* Send sound to ASIC, call once per frame */
	Sync76489(&PSG,SN76489_FLUSH);

	/* input processing */
	cnt++;
	if ((cnt % 60) == 0)
		printf(".");
}
