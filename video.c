#include <kos.h>

#include "gen-emu.h"
#include "vdp.h"

// cram is stored ---bbb-ggg-rrr-
//                edcba9876543210 
#define PACK_RGB565(r,g,b) (((r >> 3) << 11) | ((g >> 2) << 5) | ((b >> 3) << 0))
#define getr(cramval) (((cramval>>1) & 0x07)<<5)
#define getg(cramval) (((cramval>>5) & 0x07)<<5)
#define getb(cramval) (((cramval>>9) & 0x07)<<5)

uint16_t cram16[64];

extern struct vdp_s vdp;

void display_cram(void)
{
	uint32_t i;
	uint32_t x,y; 
	for(i=0;i<64;i++)
		cram16[i] = PACK_RGB565(getr(vdp.cram[i]),
					getg(vdp.cram[i]),
					getb(vdp.cram[i]));

	for(y = 0; y < 64; y++)
	{
		int i = y / 8;

		for(x = 0; x < 64; x++)
		{
			vram_s[64000 + ((y*640) + x)] = cram16[(i * 8) + (x / 8)];
		}		
	}  
}
