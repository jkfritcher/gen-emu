/* $Id$ */

#ifndef _CART_H_
#define _CART_H_

typedef struct cart_s {
	uint8		*rom;
	uint32_t	rom_len;
	uint8		*sram;
	uint32_t	sram_len;
	uint32_t	sram_start;
	uint32_t	sram_end;
	uint32_t	banks[8];
	uint8_t		banked;
} cart_t;

extern cart_t cart;

#endif /* _CART_H_ */
