/* $Id$ */

#include <kos.h>

#include "gen-emu.h"
#include "vdp.h"
#include "m68k.h"


struct vdp_s vdp;


uint16_t vdp_control_read(void)
{
	vdp.write_pending = 0;

	return (vdp.status);
}

void vdp_control_write(uint16_t val)
{
	if((val & 0xc000) == 0x8000) {
		if(!vdp.write_pending) {
			vdp.regs[(val & 0x1f00) >> 8] = (val & 0xff);
			vdp.code = 0x0000;
		}
	} else {
		if(!vdp.write_pending) {
			vdp.write_pending = 1;
			vdp.addr = ((vdp.addr & 0xc000) | (val & 0x3fff));
			vdp.code = ((vdp.code & 0x3c) | ((val & 0xc000) >> 14));
		} else {
			vdp.write_pending = 0;
			vdp.addr = ((vdp.addr & 0x3fff) | ((val & 0x0003) << 14));
			vdp.code = ((vdp.code & 0x03) | ((val & 0x00f0) >> 2));
		}
	}
}

uint16_t vdp_data_read(void)
{
	uint16_t ret = 0x0000;

	vdp.write_pending = 0;

	switch(vdp.code) {
	case 0x00:
		ret = ((uint16_t *)vdp.vram)[vdp.addr >> 1];
		break;
	case 0x04:
		ret = vdp.vsram[vdp.addr >> 1];
		break;
	case 0x08:
		ret = vdp.cram[vdp.addr >> 1];
		break;
	}

	return ret;
}

void vdp_data_write(uint16_t val)
{
	vdp.write_pending = 0;

	switch(vdp.code) {
	case 0x01:
		((uint16_t *)vdp.vram)[vdp.addr >> 1] = val;
		break;
	case 0x03:
		vdp.cram[(vdp.addr >> 1) & 0x3f] = val;
		break;
	case 0x05:
		vdp.vsram[(vdp.addr >> 1) & 0x3f] = val;
		break;

	default:
		if ((vdp.code & 0x20) && (vdp.regs[1] & 0x10)) {
			/* dma operation */
			if (((vdp.code & 0x30) == 0x20) && !(vdp.regs[23] & 0x80)) {
				uint16_t len = (vdp.regs[20] << 8) |  vdp.regs[19];
				uint32_t addr = (vdp.regs[23] << 16) |
								(vdp.regs[22] << 8) |
								 vdp.regs[21];

				/* 68k -> vdp */
				switch(vdp.code & 0x07) {
				case 0x01:
					/* vram */
					do {
						uint16_t val = m68k_read_memory_16(addr);
						if (vdp.addr & 0x01)
							__asm__ __volatile__ ("swap.b %0, %0" : "=r" (val));
						((uint16_t *)vdp.vram)[vdp.addr >> 1] = val;
						vdp.addr += vdp.regs[15];
						addr += 2;
					} while(--len);
					break;
				case 0x03:
					/* cram */
					do {
						vdp.cram[vdp.addr >> 1] = m68k_read_memory_16(addr);
						vdp.addr += vdp.regs[15];
						addr += 2;

						if(vdp.addr > 0x7f)
							break;
					} while(--len);
					break;
				case 0x05:
					/* vsram */
					do {
						vdp.vsram[vdp.addr >> 1] = m68k_read_memory_16(addr);
						vdp.addr += vdp.regs[15];
						addr += 2;

						if(vdp.addr > 0x7f)
							break;
					} while(--len);
					break;
				default:
					printf("68K->VDP DMA Error, code %d.", vdp.code);
				}
			} else
			if (((vdp.code & 0x30) == 0x20) && ((vdp.regs[23] & 0xc0) == 0x80)) {
				/* vdp fill */
				uint16_t len = ((vdp.regs[20] << 8) | vdp.regs[19]);

				vdp.vram[vdp.addr] = val & 0xff;
				val = (val >> 8) & 0xff;

				do {
					vdp.vram[vdp.addr ^ 1] = val;
					vdp.addr += vdp.regs[15];
				} while(--len);
			} else
			if ((vdp.code == 0x30) && ((vdp.regs[23] & 0xc0) == 0xc0)) {
				/* vdp copy */
				uint16_t len = (vdp.regs[20] << 8) | vdp.regs[19];
				uint16_t addr = (vdp.regs[22] << 8) | vdp.regs[21];

				do {
					vdp.vram[vdp.addr] = vdp.vram[addr++];
					vdp.addr += vdp.regs[15];
				} while(--len);
			} else {
				printf("VDP DMA Error, code %02x, r23 %02x\n", vdp.code, vdp.regs[23]);
			}
		}
	}
}
