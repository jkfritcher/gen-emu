/* $Id$ */

#include <kos.h>

#include "gen-emu.h"


static uint8_t outputs[3];
static uint8_t outputs_mask[3];
static maple_device_t *caddr[3];


void ctlr_init(void)
{
	caddr[0] = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	caddr[1] = maple_enum_type(1, MAPLE_FUNC_CONTROLLER);
	caddr[2] = NULL;
}

void ctlr_reset(void)
{
	int i;

	for(i = 0; i < 3; i++)
		outputs[i] = outputs_mask[i] = 0;
}

uint8_t ctlr_data_reg_read(int port)
{
	uint8_t ret = 0;

	if (caddr[port] != NULL) {
		cont_state_t *cont = maple_dev_status(caddr[port]);

		ret |= outputs[port];
		if (outputs[port] & 0x40) {
			ret |= (cont->buttons & CONT_B) ? 0 : 0x20;
			ret |= (cont->buttons & (CONT_A | CONT_Y)) ? 0 : 0x10;
			ret |= (cont->buttons & CONT_DPAD_RIGHT) ? 0 : 0x08;
			ret |= (cont->buttons & CONT_DPAD_LEFT) ? 0 : 0x04;
			ret |= (cont->buttons & CONT_DPAD_DOWN) ? 0 : 0x02;
			ret |= (cont->buttons & CONT_DPAD_UP) ? 0 : 0x01;
		} else {
			ret |= (cont->buttons & CONT_START) ? 0 : 0x20;
			ret |= (cont->buttons & CONT_X) ? 0 : 0x10;
			ret |= (cont->buttons & CONT_DPAD_DOWN) ? 0 : 0x02;
			ret |= (cont->buttons & CONT_DPAD_UP) ? 0 : 0x01;
		}
	} else {
		ret = (~outputs_mask[port]) & 0x7f;
		ret |= outputs[port];
	}

	return(ret);
}

void ctlr_data_reg_write(int port, uint8_t val)
{
	outputs[port] = val & (outputs_mask[port] | 0x80);
}

uint8_t ctlr_ctrl_reg_read(int port)
{
	return outputs_mask[port];
}

void ctlr_ctrl_reg_write(int port, uint8_t val)
{
	outputs_mask[port] = val;
}
