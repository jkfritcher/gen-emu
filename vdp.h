#ifndef __gen_vdp_h
#define __gen_vdp_h

#include "gen-emu.h"

#define mask_Fifo_Empty 0x200
#define mask_Fifo_Fill 0x100
#define mask_Vertical_interrupt_pending 0x80
#define mask_Sprite_overflow_on_current_scan_line 0x40
#define mask_Sprite_collision 0x20
#define mask_Odd_frame 0x10
#define mask_Vertical_blanking 0x08
#define mask_Horizontal_blanking 0x04
#define mask_DMA_in_progress 0x02
#define mask_PAL_mode_flag 0x01

struct VDP
{
	uint8_t write_pending;
	uint16_t status;
	uint16_t hv;

	uint8_t code;	
	uint16_t addr;

	uint32_t control;

	uint8_t regs[23];

	uint8_t vram[65536];
	uint16_t *vram16;
	uint16_t cram[64];
	uint16_t vsram[40];
};

//bool_t write_pending;
//uint32_t command_word;

//uint16_t data;
//uint16_t controlr;
//uint16_t control;
//uint16_t hv;
//uint8_t h;
//uint8_t v;

// status reported on control port 16-bit read
//bool_t FIFO_Empty;
//bool_t FIFO_Full;
//bool_t Vertical_interrupt_pending;
//bool_t Sprite_overflow_on_current_line;
//bool_t Sprite_collision;
//bool_t Odd_frame;
//bool_t Vertical_blanking;
//bool_t Horiztonal_blanking;
//bool_t DMA_in_progress;
//bool_t PAL_mode_flag;

//uint32_t vdp_read_8(uint32_t addr);
//uint32_t vdp_read_16(uint32_t addr);
//void vdp_write_8(uint32_t addr, uint32_t val);
//void vdp_write_16(uint32_t addr, uint32_t val);
//void vdp_execute(uint16_t cw);

uint16_t vdp_control_read(void);
uint16_t vdp_data_read(void);

#endif // __gen_vdp_h
