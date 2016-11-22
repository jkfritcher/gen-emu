#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "gen-emu.h"

#include "m68k.h"
#include "z80.h"
#include "vdp.h"
#include "SN76489.h"

#include "input.h"


char *romname = "romtoload.bin";

char *scrcapname = "/pc/home/jkf/src/dc/gen-emu/screen.ppm";

uint8_t debug = 0;
uint8_t vdp_debug = 0;
uint8_t quit = 0;
uint8_t dump = 0;
uint8_t paused = 0;

uint32_t rom_load(char *name);
void rom_free(void);
void run_one_field(void);
void gen_init(void);
void gen_reset(void);

extern void video_init();
extern void do_frame();

extern SN76489 PSG; 
extern struct vdp_s vdp;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *disp_tex;
SDL_Rect disp_srcrect = { 0, 0, 256, 224 };

uint16_t win_width = 256;
uint16_t win_height = 224;

uint16_t *pix_buf;
int pix_pitch;


int main(int argc, char *argv[])
{
	int fd;
    SDL_Event event;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
       fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
       return -1;
    }
    atexit(SDL_Quit);

    window = SDL_CreateWindow("Output",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              win_width * 4, win_height * 4,
                              SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        exit(-1);
    }

    //renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        exit(-1);
    }

    disp_tex = SDL_CreateTexture(renderer,
                                 SDL_PIXELFORMAT_ABGR4444,
                                 SDL_TEXTUREACCESS_STREAMING,
                                 512, 512);
    if (disp_tex == NULL) {
        fprintf(stderr, "SDL_CreateTexture error: %s\n", SDL_GetError());
        exit(-1);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);

	gen_init();

	rom_load(romname);

	gen_reset();

	do {
		run_one_field();

ispaused:
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYUP) {
                switch(event.key.keysym.sym) {
                case SDLK_w:
                case SDLK_s:
                case SDLK_a:
                case SDLK_d:
                case SDLK_j:
                case SDLK_k:
                case SDLK_l:
                case SDLK_u:
                case SDLK_i:
                case SDLK_o:
                case SDLK_RETURN:
                    ctlr_handle_input(event.key.keysym.sym, 1);
                    break;
                }
            }

            if (event.type == SDL_KEYDOWN) {
                       switch(event.key.keysym.sym) {
                case SDLK_w:
                case SDLK_s:
                case SDLK_a:
                case SDLK_d:
                case SDLK_j:
                case SDLK_k:
                case SDLK_l:
                case SDLK_u:
                case SDLK_i:
                case SDLK_o:
                case SDLK_RETURN:
                    ctlr_handle_input(event.key.keysym.sym, 0);
                    break;
                       case SDLK_SPACE:        /* Space */
                               paused = !paused;
                               break;
                       case SDLK_ESCAPE:       /* Escape */
                               quit = 1;
                    paused = 0;
                               break;
                       case SDLK_9:    /* 9 */
                               dump = 1;
                               break;
                case SDLK_r:
                    gen_reset();
                    break;
                case SDLK_v:
                    vdp_debug = !vdp_debug;
                    break;
                       case SDLK_PRINTSCREEN:  /* Print Screen */
                               //vid_screen_shot(scrcapname);
                               fd = open("/pc/home/jkf/src/dc/gen-emu/vdp.bin", O_WRONLY | O_TRUNC);
                               write(fd, &vdp, sizeof(vdp));
                               close(fd);
                               break;
                       }
            }
        }
               if (paused) {
            struct timespec tm = { 0, 100000000 };
            nanosleep(&tm, NULL);
                   goto ispaused;
        }
	} while (!quit);

	rom_free();

	return 0;
}

void run_one_field(void)
{
	static int cnt = 0;
	int line;

    if ((vdp.dis_cells * 8) != win_width) {
        win_width = vdp.dis_cells * 8;
        disp_srcrect.w = win_width;
        SDL_SetWindowSize(window, win_width * 4, win_height * 4);
    }

    SDL_RenderClear(renderer);

    SDL_LockTexture(disp_tex, NULL, (void **)&pix_buf, &pix_pitch);

	for(line = 0; line < 262 && !quit; line++) {
		vdp_interrupt(line);

		m68k_execute(488);
		if (z80_enabled())
			z80_execute(228);

		if (line < 224) {
			/* render scanline to vram*/
			vdp_render_scanline(line);
		}
	}

    SDL_UnlockTexture(disp_tex);

    SDL_RenderCopy(renderer, disp_tex, &disp_srcrect, NULL);

    SDL_RenderPresent(renderer);

	/* Send sound to ASIC, call once per frame */
	//Sync76489(&PSG,SN76489_FLUSH);

	/* input processing */
}
