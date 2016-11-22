/* $Id$ */

#include <SDL2/SDL.h>

#include "gen-emu.h"
#include "vdp.h"
#include "m68k.h"
#include "z80.h"
#include "cart.h"

extern uint16_t *m68k_ram16;

struct vdp_s vdp;

extern uint16_t *pix_buf;
extern int pix_pitch;

uint8_t nt_cells[4] = { 32, 64, 0, 128 };
uint8_t mode_cells[4] = { 32, 40, 0, 40 };

static uint16_t *ocr_vram = NULL;
static uint16_t pix_byte_map[4][32] = {
    {   1,   1,   0,   0,   3,   3,   2,   2,
       33,  33,  32,  32,  35,  35,  34,  34,
       65,  65,  64,  64,  67,  67,  66,  66,
       97,  97,  96,  96,  99,  99,  98,  98
    },
    {   1,   1,   0,   0,   3,   3,   2,   2,
       65,  65,  64,  64,  67,  67,  66,  66,
      129, 129, 128, 128, 131, 131, 130, 130,
      193, 193, 192, 192, 195, 195, 194, 194
    },
    {   1,   1,   0,   0,   3,   3,   2,   2,
       97,  97,  96,  96,  99,  99,  98,  98,
      193, 193, 192, 192, 195, 195, 194, 194,
      289, 289, 288, 288, 291, 291, 290, 290
    },
    {   1,   1,   0,   0,   3,   3,   2,   2,
      129, 129, 128, 128, 131, 131, 130, 130,
      257, 257, 256, 256, 259, 259, 258, 258,
      385, 385, 384, 384, 387, 387, 386, 386
    }
};

void vdp_init(void)
{
    vdp.sat_dirty = 1;
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

/* val is little endian */
void vdp_control_write(uint16_t val)
{
	if (debug)
		printf("VDP C <- %04x\n", val);

	if((val & 0xc000) == 0x8000) {
		if(!vdp.write_pending) {
			vdp.regs[(val >> 8) & 0x1f] = (val & 0xff);
			switch((val >> 8) & 0x1f) {
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
				vdp.dis_cells = mode_cells[((vdp.regs[12] & 0x01) << 1) |
											(vdp.regs[12] >> 7)];
				break;
			case 0x0d:
				vdp.hs_off = vdp.regs[13] << 10;
				break;
			case 0x10:	/* Scroll size */
				vdp.sc_width = nt_cells[vdp.regs[16] & 0x03];
				vdp.sc_height = nt_cells[(vdp.regs[16] & 0x30) >> 4];
				break;
			}
			vdp.code = 0x0000;
		}
	} else {
		if (!vdp.write_pending) {
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
						exit(-1);
					}

					/* 68k -> vdp */
					switch(vdp.code & 0x07) {
					case 0x01:
						/* vram */
						do {
							val = src_mem[src_off & src_mask];
                            if (!(vdp.addr & 0x01))
                                val = SWAPBYTES16(val);
							((uint16_t *)vdp.vram)[vdp.addr >> 1] = val;
							vdp.addr += vdp.regs[15];
							src_off += 1;
						} while(--len);
						break;
					case 0x03:
						/* cram */
						do {
							val = SWAPBYTES16(src_mem[src_off & src_mask]);
							vdp.cram[vdp.addr >> 1] = val;
							vdp.addr += vdp.regs[15];
							src_off += 1;

							if(vdp.addr > 0x7f)
								break;
						} while(--len);
						for(len = 0; len < 64; len++) {
							vdp.dc_cram[len] = vdp.cram[len];
						}
						break;
					case 0x05:
						/* vsram */
						do {
							val = SWAPBYTES16(src_mem[src_off & src_mask]);
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
		vdp.dc_cram[(vdp.addr >> 1) & 0x3f] = val;
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
	//sq_cpy(cram_txr, vdp.dc_cram, 128);
}

void vdp_render_plane(int line, int plane, int priority)
{
	int row, pixrow, i, j;
	sint16_t hscroll = 0;
	sint8_t  col_off, pix_off, pix_tmp;
	uint16_t *p;

	p = plane ? vdp.bgb : vdp.bga;

	switch(vdp.regs[11] & 0x03) {
	case 0x0:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (plane ? 2 : 0)) >> 1];
		break;
	case 0x1:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & 0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x2:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & ~0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x3:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (line << 2) + (plane ? 2 : 0)) >> 1];
		break;
	}

	hscroll = (0x400 - hscroll) & 0x3ff;
	col_off = hscroll >> 3;
	pix_off = hscroll & 0x7;
	pix_tmp = pix_off;

	if ((vdp.regs[11] & 0x04) == 0)
		line = (line + (vdp.vsram[(plane ? 1 : 0)] & 0x3ff)) % (vdp.sc_height << 3);

	row = (line / 8) * vdp.sc_width;
	pixrow = line % 8;

	i = 0;
	while (i < (vdp.dis_cells * 8)) {
		uint16_t name_ent = p[row + ((col_off + ((pix_off + i) >> 3)) % vdp.sc_width)];

		if ((name_ent >> 15) == priority) {
			uint32_t data;
			sint32_t pal, pixel;

			pal = (name_ent >> 9) & 0x30;

			if ((name_ent >> 12) & 0x1)
				data = *(uint32_t *)(vdp.vram + ((name_ent & 0x7ff) << 5) + (28 - (pixrow * 4)));
			else
				data = *(uint32_t *)(vdp.vram + ((name_ent & 0x7ff) << 5) + (pixrow * 4));
			data = SWAP_WORDS(data);

			if ((name_ent >> 11) & 0x1) {
				for (j = 0; j < 8; j++) {
					pixel = data & 0x0f;
					data >>= 4;
					if (pix_tmp > 0) {
						pix_tmp--;
						continue;
					}
					if (pixel)
						ocr_vram[i] = vdp.dc_cram[pal | pixel];
					i++;
				}
			} else {
				for (j = 0; j < 8; j++) {
					pixel = data >> 28;
					data <<= 4;
					if (pix_tmp > 0) {
						pix_tmp--;
						continue;
					}
					if (pixel)
						ocr_vram[i] = vdp.dc_cram[pal | pixel];
					i++;
				}
			}
		} else {
			if (pix_tmp > 0) {
				i += (8 - pix_tmp);
				pix_tmp = 0;
			} else {
				i += 8;
			}
		}
	}
}

struct spr_ent_s {
    int16_t y;
    uint8_t v;
    uint8_t h;
    uint8_t l;
    uint16_t n;
    uint8_t hf;
    uint8_t vf;
    uint8_t pal;
    uint8_t prio;
    int16_t x;
};

void vdp_render_sprites2(int line, int priority)
{
    static struct spr_ent_s spr_list[80] = { {0} };
    static uint8_t spr_list_len = 0;
    static uint8_t spr_this_field = 0;

    uint8_t max_sprites = vdp.dis_cells == 40 ? 80 : 64;

    if (line == 0 && priority == 0) {
        spr_this_field = 0;
        int i = 0, j = 0;
        while (j < max_sprites && i < max_sprites) {
            uint8_t *spr_ent_raw = (uint8_t *)&(vdp.sat[i]);
            struct spr_ent_s *spr_ent = &(spr_list[j++]);
            spr_ent->y = (((spr_ent_raw[1] & 0x03) << 8) | spr_ent_raw[0]) - 128;
            spr_ent->v = (spr_ent_raw[3] & 0x03) + 1;
            spr_ent->h = ((spr_ent_raw[3] & 0x0c) >> 2) + 1;
            spr_ent->l = spr_ent_raw[2] & 0x7f;

            spr_ent->n = (((spr_ent_raw[5] & 0x07) << 8) | spr_ent_raw[4]) << 5;
            spr_ent->hf = (spr_ent_raw[5] & 0x08) >> 3;
            spr_ent->vf = (spr_ent_raw[5] & 0x10) >> 4;
            spr_ent->pal = (spr_ent_raw[5] & 0x60) >> 1;
            spr_ent->prio = (spr_ent_raw[5] & 0x80) >> 7;
            spr_ent->x = (((spr_ent_raw[7] & 0x03) << 8) | spr_ent_raw[6]) - 128;
            if (vdp_debug) {
                //printf("%02x%02x%02x%02x%02x%02x%02x%02x ",
                //       spr_ent_raw[1], spr_ent_raw[0], spr_ent_raw[3], spr_ent_raw[2],
                //       spr_ent_raw[5], spr_ent_raw[4], spr_ent_raw[7], spr_ent_raw[6]);
                printf("y=%d, v=%d, h=%d, l=%d, n=%d, hf=%d, vf=%d, pal=%d, prio=%d, x=%d\n",
                      spr_ent->y, spr_ent->v, spr_ent->h, spr_ent->l, spr_ent->n,
                      spr_ent->hf, spr_ent->vf, spr_ent->pal, spr_ent->prio, spr_ent->x);
            }
            i = spr_ent->l;
            if (i == 0)
                break;
        }
        spr_list_len = j;
    }

    uint8_t max_spr_per_line = vdp.dis_cells / 2;
    uint16_t max_spr_pix_per_line = vdp.dis_cells * 8;
    struct spr_ent_s *spr_to_render[20] = { 0 };
    uint8_t spr_to_render_len = 0;

    uint8_t pix_overflow = 0;
    /* Walk sprite list to find what spites we need to render */
    for (int i = 0, j = 0, k = 0; j < max_spr_per_line && k < max_spr_pix_per_line && spr_this_field < max_sprites && i < spr_list_len;) {
        struct spr_ent_s *spr_ent = &(spr_list[i++]);

        /* Contains our scanline? */
        if ((line < spr_ent->y) || (line >= (spr_ent->y + (spr_ent->v * 8)))) {
            continue;
        }

        /* Masking? */
        if (spr_ent->x == -128) {
            break;
        }

        /* Priority match? */
        if (spr_ent->prio != priority) {
            continue;
        }

        spr_to_render[j++] = spr_ent;
        spr_to_render_len++;
        k += spr_ent->h * 8;
        if (k > max_spr_pix_per_line) {
            pix_overflow = k - max_spr_pix_per_line;
        }
    }

    /* Render sprites back to front */
    for (int i = spr_to_render_len - 1; i >= 0; i--) {
        struct spr_ent_s *spr_ent = spr_to_render[i];
        uint8_t spr_pat_width = spr_ent->h * 8;

        /* Offscreen? */
        if (spr_ent->x < -(spr_pat_width - 1) ||
            spr_ent->x > max_spr_pix_per_line) {
            if (pix_overflow > 0) {
                pix_overflow = 0;
            }
            continue;
        }

        uint8_t spr_pat_height = spr_ent->v * 8;
        uint8_t spr_pat_line, spr_pat_vskip;
        if (!spr_ent->vf) {
            spr_pat_line = (line - spr_ent->y) % 8;
            spr_pat_vskip = (line - spr_ent->y) / 8;
        } else {
            spr_pat_line = (spr_pat_height - 1 - (line - spr_ent->y)) % 8;
            spr_pat_vskip = (spr_pat_height - 1 - (line - spr_ent->y)) / 8;
        }
        uint16_t spr_pat_voff = spr_pat_vskip * 32 + spr_pat_line * 4;

        uint8_t *pixels = vdp.vram + spr_ent->n + spr_pat_voff;
        uint8_t pal = spr_ent->pal;
        uint8_t pat_stride_idx = spr_ent->v - 1;

        if (pix_overflow > 0) {
            spr_pat_width -= pix_overflow;
            pix_overflow = 0;
        }
        for (int x = spr_ent->x, i = 0; i < spr_pat_width && x < max_spr_pix_per_line; i++, x++) {
            if (x < 0) {
                continue;
            }

            uint8_t pixel = 0;
            if (!spr_ent->hf) {
                pixel = pixels[pix_byte_map[pat_stride_idx][i]];
                pixel = i % 2 == 0 ? (pixel & 0xf0) >> 4 : pixel & 0x0f;
            } else {
                pixel = pixels[pix_byte_map[pat_stride_idx][spr_pat_width-1-i]];
                pixel = i % 2 == 1 ? (pixel & 0xf0) >> 4 : pixel & 0x0f;
            }

            if (pixel)
                ocr_vram[x] = vdp.dc_cram[pal | pixel];
        }
    }
}

#define spr_start (vdp.vram + sn)

void vdp_render_sprites1(int line, int priority)
{
    uint8_t pixel;
    uint16_t *pal;
    uint32_t data;
    uint32_t spr_ent_bot,spr_ent_top;
    uint32_t c=0, cells=64, i=0, j, k, h,v, sp, sl, sh, sv, sn, sc, shf, svf;
    uint32_t dis_line=16;
    uint32_t ppl=0;
    uint32_t dis_ppl=256;
    uint32_t sol=0;  
    sint32_t sx, sy;
    sint32_t list_ordered[80];
    uint64_t spr_ent;

    for(j=0;j<80;++j)
        list_ordered[j]=-1;

    if (!(vdp.dis_cells == 32))
        cells = 80;

    if(cells == 80)
    {
        dis_line=20;	
        dis_ppl=320;
    }

    vdp.status &= 0x0040; // not too sure about this... 

    for(i=0;i<cells;++i)
    {
		spr_ent = vdp.sat[c];

        spr_ent_bot = SWAP_WORDS(spr_ent >> 32);
        spr_ent_top = SWAP_WORDS(spr_ent & 0x00000000ffffffff);

        sy = ((spr_ent_top & 0x03FF0000) >> 16)-128;
        sh = ((spr_ent_top & 0x00000C00) >> 10)+1;
        sv = ((spr_ent_top & 0x00000300) >> 8)+1;

        if((line >= sy) && (line < (sy+(sv<<3)))) 
        {
            sp = (spr_ent_bot & 0x80000000) >> 31;

            if(sp == priority) 
            {
	        	list_ordered[i]=c;
	    	}

            sol++;
            ppl+=(sh<<3);

            if(!(sol < dis_line))
            {
     	        vdp.status |= 0x0040;
				goto end;
            }			

	    	if(!(ppl < dis_ppl))
	    	{
	        	goto end;
            } 		
        }

        sl = (spr_ent_top & 0x0000007F);	
        if(sl)
            c = sl;
        else
end:
            break;			
    }

    for(i=0;i<cells;i++)
    {
        if( list_ordered[ ( 79 - ( (cells==64)?16:0 ) - i ) ] > -1 )
        {
	    	spr_ent = vdp.sat[list_ordered[(79-((cells==64)?16:0)-i)]];
            spr_ent_bot = SWAP_WORDS(spr_ent >> 32);
            spr_ent_top = SWAP_WORDS(spr_ent & 0x00000000ffffffff);

            sy = ((spr_ent_top & 0x03FF0000) >> 16)-128;
            sh = ((spr_ent_top & 0x00000C00) >> 10)+1;
            sv = ((spr_ent_top & 0x00000300) >> 8)+1;
            svf = (spr_ent_bot & 0x10000000) >> 28;
            shf = (spr_ent_bot & 0x08000000) >> 27;
            sn = (spr_ent_bot & 0x07FF0000) >> 11;
            sx = (spr_ent_bot & 0x000003FF)-128;		
            sc = (spr_ent_bot & 0x60000000) >> 29;
            pal = vdp.dc_cram + (sc << 4);	 

            if (sx < -31 || sy < -31)
                continue;

            for(v = 0; v < sv; ++v) 
            {
                for(k=0;k<8;k++)
        		{
                    if((sy+(v<<3)+k) == line) 
                    {
                        for(h = 0; h < sh; ++h) 
                        {
                            if (!svf) 
    						{
                                if(shf)
                                    data = *(uint32_t *)(spr_start + (((sv*(sh-h-1))+v)<<5) + (k << 2));
                                else
                                    data = *(uint32_t *)(spr_start + (((sv*h)+v)<<5) + (k << 2));
                            }
                            else 
    						{
                                if(shf)
                                    data = *(uint32_t *)(spr_start + (((sv*(sh-h-1))+(sv-v-1))<<5) + (28 - (k << 2)));
                                else
                                    data = *(uint32_t *)(spr_start + (((sv*h)+(sv-v-1))<<5) + (28 - (k << 2)));
                            }
                            data = SWAP_WORDS(data);

                            if (shf) 
                            {
								for(j=0;j<8;j++)
                                {
                                    pixel = data & 0x0f;
                                    data >>= 4;
                                    if (pixel)	
                                        ocr_vram[sx + j + (h<<3)] = pal[pixel];
                                }
                            }
                            else 
                            {
								for(j=0;j<8;j++)
                                {
                                    pixel = data >> 28;
                                    data <<= 4;
                                    if (pixel)
                                        ocr_vram[sx + j + (h<<3)] = pal[pixel];
                                }
                            }
                        } 
                    } 
                } 
            } 
        }       
    }
}

void vdp_render_scanline(int line)
{
	ocr_vram = (pix_buf + (line * pix_pitch / 2));

	/* Prefill the scanline with the backdrop color. */
	for (int i = 0; i < (vdp.sc_width * 8); i++)
		ocr_vram[i] = vdp.dc_cram[vdp.regs[7] & 0x3f];

	/* Render scanline */
	if (vdp.regs[1] & 0x40) {
		vdp_render_plane(line, 1, 0);
		vdp_render_plane(line, 0, 0);
		//vdp_render_sprites1(line, 0);
		vdp_render_sprites2(line, 0);
		vdp_render_plane(line, 1, 1);
		vdp_render_plane(line, 0, 1);
		//vdp_render_sprites1(line, 1);
		vdp_render_sprites2(line, 1);
	}
}
