/* $Id$ */

#ifndef _M68K_H_
#define _M68K_H_

#include "m68k/m68k.h"

extern uint32_t m68k_read_memory_8(uint32_t addr);
extern uint32_t m68k_read_memory_16(uint32_t addr);
extern uint32_t m68k_read_memory_32(uint32_t addr);

extern void m68k_write_memory_8(uint32_t addr, uint32_t val);
extern void m68k_write_memory_16(uint32_t addr, uint32_t val);
extern void m68k_write_memory_32(uint32_t addr, uint32_t val);

extern uint8_t m68k_ram[65536];

#endif /* _M68K_H_ */
