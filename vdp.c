/* $Id$ */

#include <kos.h>

#include "gen-emu.h"
#include "vdp.h"
#include "m68k.h"
#include "z80.h"


struct vdp_s vdp;


uint16_t vdp_control_read(void)
{
	uint16_t ret = 0x3500;

	vdp.write_pending = 0;
	m68k_set_irq(0);


	ret |= (vdp.status & 0x00ff);

	if (debug)
		printf("VDP C -> %04x\n", ret);

	return ret;
}

void vdp_control_write(uint16_t val)
{
	if (debug)
		printf("VDP C <- %04x\n", val);

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

			if ((vdp.code & 0x20) && (vdp.regs[1] & 0x10)) {
				/* dma operation */
				if (((vdp.code & 0x30) == 0x20) && !(vdp.regs[23] & 0x80)) {
					uint16_t len = (vdp.regs[20] << 8) |  vdp.regs[19];
					uint32_t addr = ((vdp.regs[23] << 16) |
									(vdp.regs[22] << 8) |
									 vdp.regs[21]) << 1;

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
				}
			}
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

	if (debug)
		printf("VDP D -> %04x\n", ret);

	vdp.addr += 2;

	return ret;
}

void vdp_data_write(uint16_t val)
{
	vdp.write_pending = 0;

	if (debug)
		printf("VDP D <- %04x\n", val);

	switch(vdp.code) {
	case 0x01:
		((uint16_t *)vdp.vram)[vdp.addr >> 1] = val;
		vdp.addr += 2;
		break;
	case 0x03:
		vdp.cram[(vdp.addr >> 1) & 0x3f] = val;
		vdp.addr += 2;
		break;
	case 0x05:
		vdp.vsram[(vdp.addr >> 1) & 0x3f] = val;
		vdp.addr += 2;
		break;

	default:
		if ((vdp.code & 0x20) && (vdp.regs[1] & 0x10)) {
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

uint16_t vdp_hv_read(void)
{
	uint16_t h, v;

	v = vdp.scanline;
	if ( v > 0xea)
		v -= 6;

	h = (uint16_t)(m68k_cycles_run() * 0.70082f);
	if (h > 0xe9)
		h -= 86;

	return(((v & 0xff) << 8) | (h & 0xff));
}

void vdp_interrupt(int line)
{
	uint8_t h_int_pending = 0;

	vdp.scanline = line;

	if (vdp.h_int_counter == 0) {
		vdp.h_int_counter = vdp.regs[10];
		if (vdp.regs[0] & 0x10)
			h_int_pending = 1;
	}

	if (line < 224) {
		if (line == 0) {
			vdp.h_int_counter = vdp.regs[10];
			vdp.status &= 0x0001;
		}

		if (h_int_pending) {
			m68k_set_irq(4);
		}
	} else
	if (line == 224) {
		z80_set_irq_line(0, PULSE_LINE);
		if (h_int_pending) {
			m68k_set_irq(4);
		} else {
			if (vdp.regs[1] & 0x20) {
				vdp.status |= 0x0080;
				m68k_set_irq(6);
			}
		}
	} else {
		vdp.h_int_counter = vdp.regs[10];
		vdp.status |= 0x08;
	}

	vdp.h_int_counter--;
}
