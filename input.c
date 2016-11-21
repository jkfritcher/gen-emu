/* $Id$ */

#include <SDL2/SDL.h>

#include "gen-emu.h"


struct controller_s {
    uint8_t buttons;
    uint8_t dpad;
};

static uint8_t outputs[3];
static uint8_t outputs_mask[3];
static struct controller_s *caddr[3];


void ctlr_init(void)
{
    if (caddr[0] != NULL)
        free(caddr[0]);
    caddr[0] = calloc(1, sizeof(struct controller_s));
    if (caddr[1] != NULL)
        free(caddr[1]);
    caddr[1] = NULL;
    if (caddr[2] != NULL)
        free(caddr[2]);
	caddr[2] = NULL;
}

void ctlr_reset(void)
{
	int i;

	for(i = 0; i < 3; i++)
		outputs[i] = outputs_mask[i] = 0;
}

void ctlr_handle_input(int sym, uint8_t up)
{
    switch(sym) {
        case SDLK_RETURN:
            if (!up) {
                caddr[0]->buttons |= 0x40;
            } else {
                caddr[0]->buttons &= ~0x40;
            }
            break;
    }
}

uint8_t ctlr_data_reg_read(int port)
{
	uint8_t ret = 0;

	if (caddr[port] != NULL) {
		ret |= outputs[port];
		if (outputs[port] & 0x40) {
/*
			ret |= (cont->buttons & CONT_B) ? 0 : 0x20;
			ret |= (cont->buttons & (CONT_A | CONT_Y)) ? 0 : 0x10;
			ret |= (cont->buttons & CONT_DPAD_RIGHT) ? 0 : 0x08;
			ret |= (cont->buttons & CONT_DPAD_LEFT) ? 0 : 0x04;
			ret |= (cont->buttons & CONT_DPAD_DOWN) ? 0 : 0x02;
			ret |= (cont->buttons & CONT_DPAD_UP) ? 0 : 0x01;
*/
            ret |= 0x3f;
		} else {
/*
			ret |= (cont->buttons & CONT_START) ? 0 : 0x20;
			ret |= (cont->buttons & CONT_X) ? 0 : 0x10;
			ret |= (cont->buttons & CONT_DPAD_DOWN) ? 0 : 0x02;
			ret |= (cont->buttons & CONT_DPAD_UP) ? 0 : 0x01;
*/
			ret |= (caddr[port]->buttons & 0x40) ? 0 : 0x20;   // Start
            ret |= 0x13;
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
