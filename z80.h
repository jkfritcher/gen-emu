/* $Id$ */

#ifndef _Z80_H_
#define _Z80_H_

#include "mz80/mz80.h"

extern uint8_t mz80_ram[8192];
extern uint32_t z80_bank_base;
extern uint8_t z80_bank_shift;
extern uint8_t z80_running;
extern uint8_t z80_busreq;

extern uint32_t z80_init(void);
extern uint32_t z80_enabled(void);
extern UINT8 mz80_read_memory(UINT32, struct MemoryReadByte *);
extern void mz80_write_memory(UINT32, UINT8, struct MemoryWriteByte *);

#endif /* _Z80_H_ */
