/* $Id$ */

#include <kos.h>

#include "gen-emu.h"
#include "vdp.h"
#include "m68k.h"
#include "z80.h"
#include "cart.h"

extern uint16_t *m68k_ram16;

struct vdp_s vdp;
pvr_poly_hdr_t disp_hdr;
pvr_ptr_t disp_txr;
pvr_poly_hdr_t cram_hdr;
pvr_ptr_t cram_txr;

uint8_t nt_cells[4] = { 32, 64, 0, 128 };
uint8_t mode_cells[4] = { 32, 40, 0, 40 };


static uint16_t *ocr_vram = (uint16_t *)0x7c002000;


void vdp_init(void)
{
	pvr_poly_cxt_t cxt;

	/* Allocate and build display texture data */
	disp_txr = pvr_mem_malloc(512 * 256 * 2);
	pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY,
		PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED,
		512, 256, disp_txr, PVR_FILTER_NONE);
	pvr_poly_compile(&disp_hdr, &cxt);

	/* Allocate and build cram texture data */
	cram_txr = pvr_mem_malloc(64 * 64 * 2);
	pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY,
		PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED,
		64, 64, cram_txr, PVR_FILTER_NONE);
	pvr_poly_compile(&cram_hdr, &cxt);
}

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
			switch((val & 0x1f00) >> 8) {
			case 0x02:	/* BGA */
				vdp.bga = (uint16_t *)(vdp.vram + ((val & 0x38) << 10));
				break;
			case 0x03:	/* WND */
				vdp.wnd = (uint16_t *)(vdp.vram + ((val & 0x3e) << 10));
				break;
			case 0x04:	/* BGB */
				vdp.bgb = (uint16_t *)(vdp.vram + ((val & 0x07) << 13));
				break;
			case 0x05:	/* SAT */
				vdp.sat = (uint64_t *)(vdp.vram + ((val & 0x7f) << 9));
				break;
			case 0x0c:	/* Mode Set #4 */
				vdp.dis_cells =  mode_cells[((vdp.regs[12] & 0x80) >> 7) |
											((vdp.regs[12] & 0x01) << 1)];
				break;
			case 0x10:	/* Scroll size */
				vdp.sc_width = nt_cells[vdp.regs[16] & 0x03];
				vdp.sc_height = nt_cells[(vdp.regs[16] & 0x30) >> 4];
				break;
			}
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
					uint16_t src_off = (vdp.regs[22] << 8) | vdp.regs[21];
					uint16_t *src_mem, src_mask;

					if ((vdp.regs[23] & 0x70) == 0x70) {
						src_mem = m68k_ram16;
						src_mask = 0x7fff;
					} else
					if ((vdp.regs[23] & 0x70) < 0x20) {
						src_mem = (uint16_t *)(cart.rom + (vdp.regs[23] << 17));
						src_mask = 0xffff;
					} else {
						printf("DMA from an unknown block... 0x%02x\n", (vdp.regs[23] << 1));
						arch_abort();
					}

					/* 68k -> vdp */
					switch(vdp.code & 0x07) {
					case 0x01:
						/* vram */
						do {
							val = src_mem[src_off & src_mask];
							SWAPBYTES(val);
							if (vdp.addr & 0x01)
								SWAPBYTES(val);
							((uint16_t *)vdp.vram)[vdp.addr >> 1] = val;
							vdp.addr += vdp.regs[15];
							src_off += 1;
						} while(--len);
						break;
					case 0x03:
						/* cram */
						do {
							val = src_mem[src_off & src_mask];
							SWAPBYTES(val);
							vdp.cram[vdp.addr >> 1] = val;
							vdp.addr += vdp.regs[15];
							src_off += 1;

							if(vdp.addr > 0x7f)
								break;
						} while(--len);
						for(len = 0; len < 64; len++)
							vdp.dc_cram[len] = 
								(((vdp.cram[len] & 0x000e) << 12) |
								 ((vdp.cram[len] & 0x00e0) << 3) |
								 ((vdp.cram[len] & 0x0e00) >> 7));
						break;
					case 0x05:
						/* vsram */
						do {
							val = src_mem[src_off & src_mask];
							SWAPBYTES(val);
							vdp.vsram[vdp.addr >> 1] = val;
							vdp.addr += vdp.regs[15];
							src_off += 1;

							if(vdp.addr > 0x7f)
								break;
						} while(--len);
						break;
					default:
						printf("68K->VDP DMA Error, code %d.", vdp.code);
					}

					vdp.regs[19] = vdp.regs[20] = 0;
					vdp.regs[21] = src_off & 0xff;
					vdp.regs[22] = src_off >> 8;
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
		vdp.dc_cram[(vdp.addr >> 1) & 0x3f] = 
			(((val & 0x000e) << 12) |
			 ((val & 0x00e0) << 3) |
			 ((val & 0x0e00) >> 7));
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


void vdp_render_cram(void)
{
    uint32_t x, y, i;

    for(y = 0; y < 64; y++) {
        i = y / 8;

        for(x = 0; x < 64; x++)
            ocr_vram[x] = vdp.dc_cram[(i * 8) + (x / 8)];

        sq_cpy((((uint16_t *)cram_txr) + (y * 64)), ocr_vram, 128);
    }
}

void vdp_render_plane(int line, int plane, int priority)
{
	int row, pixrow, i, j;

	row = (line / 8) * vdp.sc_width;
	pixrow = line % 8;

	/* Prefetch the needed name table row for use below. */
	for (i = 0; i < ((vdp.sc_width * 2) >> 5); i++)
		__asm__ volatile ("pref @%0" : : "r" (&vdp.bgb[row+i*16]));

	for(i = 0; i < vdp.dis_cells; i++) {
		uint16_t name_ent = vdp.bgb[row + i];
		uint16_t ocr_off = i * 8;
		uint8_t pixel;

		if ((name_ent & 0x8000) == 0) {
			uint16_t *pal = vdp.dc_cram + (((name_ent >> 13) & 0x0003) << 4);
			uint32_t data = *(uint32_t *)(vdp.vram + ((name_ent & 0x7ff) << 5) + (pixrow * 4));

			for (j = 0; j < 8; j++) {
				pixel = data >> 28;
				data <<= 4;

				if (pixel)
					ocr_vram[ocr_off + j] = pal[pixel];
			}
		}
	}
}

void vdp_render_scanline(int line)
{
	int i;

	/* Prefill the scanline with the backdrop color. */
	for (i = 0; i < (vdp.sc_width * 8); i++)
		ocr_vram[i] = vdp.dc_cram[vdp.regs[7] & 0x3f];

	vdp_render_plane(line, 1, 0);

	sq_cpy((((uint16_t *)disp_txr) + (line * 512)), ocr_vram, (vdp.dis_cells * 8 * 2));
}
