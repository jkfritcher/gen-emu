#include <kos.h>

#include "gen-emu.h"


/* PVR data from vdp.c */
extern pvr_poly_hdr_t disp_hdr;
extern pvr_ptr_t disp_txr;
extern pvr_poly_hdr_t cram_hdr;
extern pvr_ptr_t cram_txr;


void do_frame()
{
	pvr_vertex_t vert;
	int x, y, w, h;

	vert.argb = 0xffffffff;
	vert.oargb = 0x00000000;
	vert.z = 1;

	pvr_wait_ready();
	pvr_scene_begin();
	pvr_list_begin(PVR_LIST_OP_POLY);

	/* Main display */
#if 1
	x = 25; y = 25;
	w = 320; h = 240;
#else
	x = 0; y = 0;
	w = 640; h = 480;
#endif

	pvr_prim(&disp_hdr, sizeof(disp_hdr));
	vert.flags = PVR_CMD_VERTEX;
	vert.x = x;
	vert.y = y + h;
	vert.u = 0.0f;
	vert.v = 240.0f/256.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x + w;
	vert.y = y + h;
	vert.u = 320.0f/512.0f;
	vert.v = 240.0f/256.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));


	/* CRAM display */
	x = 550; y = 25;
	w = 64; h = 64;

	pvr_prim(&cram_hdr, sizeof(cram_hdr));
	vert.flags = PVR_CMD_VERTEX;
	vert.x = x;
	vert.y = y + h;
	vert.u = 0.0f;
	vert.v = 1.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x + w;
	vert.y = y + h;
	vert.u = 1.0f;
	vert.v = 1.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	pvr_list_finish();
	pvr_scene_finish();
}
