
#include <SDL2/SDL.h>

#include "gen-emu.h"
#include "m68k.h"
#include "z80.h"
#include "vdp.h"
#include "input.h"
#include "SN76489.h"


//extern SN76489 PSG; 

void gen_init(void)
{
	vdp_init();
	ctlr_init();
}

void gen_reset(void)
{
	m68k_pulse_reset();
	z80_init();
	z80_set_irq_callback(z80_irq_callback);

//	Reset76489(&PSG, 0);
//	Sync76489(&PSG, SN76489_SYNC);

	ctlr_reset();
}
