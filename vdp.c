#include <kos.h>

#include "gen-emu.h"
#include "vdp.h"

uint16_t vdp_control_read(void)
{
	uint16_t ret = vdp->status;

	vdp->write_pending = false;
	
	return (ret);
}

void vdp_control_write(uint16_t val)
{
	uint16_t tmpv = val;

	if(tmpv & 0x8000)
	{
		if(vdp->write_pending == false)
		{
			vdp->regs[ (tmpv & 0x1f00) >> 8 ] = (tmpv & 0xff);
			vdp->code = 0x0000;
		}
	}

	else
	{
		if(vdp->write_pending == false)
		{
			vdp->write_pending = true;
			vdp->control = tmpv;
		}
		
		if(vdp->write_pending == true) 
		{
			uint8_t tmp0;
			uint8_t tmp1;
			uint16_t tmp2; 
			uint16_t tmp3;

			vdp->write_pending = false;
			vdp->control |= ( ((uint32_t)tmpv) << 16);

			tmp0 = ((vdp->control & 0xf0) >> 2);
			tmp1 = (vdp->control >> 30);
			vdp->code = (tmp0 | tmp1);

			tmp2 = ((vdp->control >> 16) & 0x3fff);
			tmp3 = ((vdp->control & 0x3) << 14);
			vdp->addr = (tmp3 | tmp2);
		}
	}
}

uint16_t vdp_data_read(void)
{
	uint16_t ret = 0x0000;

	vdp->write_pending = false;

	if(vdp->code == 0x00)
	{
		while(vdp->addr > 0xffff)
			vdp->addr-=0xffff;
		ret = vdp->vram16[vdp->addr & 0xfffe];
	}

	if(vdp->code == 0x04)
	{
		while(vdp->addr > 0x7f)
			vdp->addr-=0x7f;
		ret = vdp->vsram[vdp->addr & 0x7e];
	}

	if(vdp->code == 0x08)
	{
		while(vdp->addr > 0x7f)
			vdp->addr-=0x7f;
		ret = vdp->cram[vdp->addr & 0x7e];
	}

	return ret;
}

void vdp_data_write(uint16_t val)
{
	if(vdp->code == 0x01)
	{
		while(vdp->addr > 0xffff)
			vdp->addr-=0xffff;
		vdp->vram16[vdp->addr & 0xfffe] = val;
	}

	if(vdp->code == 0x03)
	{
		while(vdp->addr > 0x7f)
			vdp->addr-=0x7f;
		vdp->cram[vdp->addr & 0x7e] = val;
	}

	if(vdp->code == 0x05)
	{
		while(vdp->addr > 0x7f)
			vdp->addr-=0x7f;
		vdp->vsram[vdp->addr & 0x7e] = val;
	}

	if(vdp->code == 0x21)
	{
		if(vdp->regs[23] & 0x20)
		{
			// vram fill
			uint16_t length = ((((short)vdp->regs[20])<<8) | vdp->regs[19]);			

			vdp->vram[vdp->addr] = (val>>0) & 0xff;

			do
			{
				vdp->vram[vdp->addr ^ 1] = (val>>8) & 0xff;
				vdp->addr += vdp->regs[15];
			}
			while(length--);			
		}
		else
		{
			// let us transfer from 68k->vram:
			// 68k is frozen
			// vdp reads a word from source address
			// source address is incremented by two
			// vdp writes word to vram
			// (data is byteswapped if addr reg bit 0 is set)
			// address register is incremented by value in reg 15
			// repeat until length counter has expired
			// 68000 resumes operation
			uint16_t length = ((((short)vdp->regs[20])<<8) | vdp->regs[19]);			
			uint32_t addr = (((long)vdp->regs[23])<<16) | (((short)vdp->regs[22])<<8) | vdp->regs[21];
			
			do
			{
				// if(addr%1), then byteswap the read
				uint16_t *rom16 = (uint16_t *)&cart->rom[addr];
				vdp->vram16[ vdp->addr ] = rom16[0];
				addr += 2;
				vdp->addr += vdp->regs[15];
			}
			while(length--);
 		}
	}

	if(vdp->code == 0x23)
	{
		// let us transfer from 68k->cram:
		// 68k is frozen
		// vdp reads a word from source address
		// source address is incremented by two
		// vdp writes word to cram
		// address register is incremented by value in reg 15
		// repeat until length counter has expired
		// 68000 resumes operation
		uint16_t length = ((((short)vdp->regs[20])<<8) |  vdp->regs[19]);			
		uint32_t addr = (((long)vdp->regs[23])<<16) | (((short)vdp->regs[22])<<8) | vdp->regs[21];

		do
		{
			// if(addr%1), then byteswap the read
			uint16_t *rom16 = (uint16_t *)&cart->rom[addr];
			vdp->cram[ vdp->addr ] = rom16[0];
			addr += 2;
			vdp->addr += vdp->regs[15];
			
			if(vdp->addr > 0x7f) break;
		}
		while(length--);
	}

	if(vdp->code == 0x25)
	{
		// let us transfer from 68k->vsram:
		// 68k is frozen
		// vdp reads a word from source address
		// source address is incremented by two
		// vdp writes word to vsram
		// address register is incremented by value in reg 15
		// repeat until length counter has expired
		// 68000 resumes operation
		uint16_t length = ((((short)vdp->regs[20])<<8) |  vdp->regs[19]);			
		uint32_t addr = (((long)vdp->regs[23])<<16) | (((short)vdp->regs[22])<<8) | vdp->regs[21];

		do
		{
			// if(addr%1), then byteswap the read
			uint16_t *rom16 = (uint16_t *)&cart->rom[addr];
			vdp->vsram[ vdp->addr ] = rom16[0];
			addr += 2;
			vdp->addr += vdp->regs[15];
			
			if(vdp->addr > 0x7f) break;
		}
		while(length--);
	}
	
	if(vdp->code == 0x30)
	{
		// vram copy
		uint16_t length = ((((short)vdp->regs[20])<<8) |  vdp->regs[19]);			
		uint16_t addr = (((short)vdp->regs[22])<<8) | vdp->regs[21];

		do
		{
			vdp->vram[vdp->addr] = vdp->vram[addr];
			addr += 1;
			vdp->addr += vdp->regs[15]; 
		}
		while(length--);
	}	
}
