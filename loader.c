
/* $Id$ */

#include <kos.h>

#include "gen-emu.h"


uint8_t *rom_load(char *name)
{
	uint8_t *rom;
	uint32_t fd, len, cksum, i;

	fd = fs_open(name, O_RDONLY);
	if (fd == 0) {
		printf("rom_load(): fs_open() failed.\n");
		goto error;
	}

	len = fs_seek(fd, 0, SEEK_END);
	fs_seek(fd, 0, SEEK_SET);

	if (debug)
		rom = malloc(4 * 1024 * 1024);
	if (strstr(name, ".bin") != NULL) {
		if (!debug)
			rom = malloc(len);
		if (rom == NULL) {
			printf("rom_load(): malloc() failed.\n");
			goto error;
		}

		fs_read(fd, rom, len);
	} else
	if (strstr(name, ".smd") != NULL) {
		uint8_t buf[16384];

		len -= 512;
		if (!debug)
			rom = malloc(len);
		if (rom == NULL) {
			printf("rom_load(): malloc() failed.\n");
			goto error;
		}

		fs_read(fd, buf, 512);
		if ((buf[8] == 0xaa) && (buf[9] == 0xbb)) {
			uint32_t blocks;
			uint8_t *tmp;
			blocks = len / 16384;

			tmp = rom;
			for(i = 0; i < blocks; i++) {
				uint32_t j;

				fs_read(fd, buf, 16384);
				for(j = 0; j < 8192; j++) {
					*tmp++ = buf[j+8192];
					*tmp++ = buf[j];
				}
			}
		}
	} else {
		printf("ROM is not a recognized format.\n");
		goto error;
	}

	fs_close(fd);

	fd = fs_open("/pc/home/jkf/src/dc/gen-emu/roms/rom.bin", O_WRONLY);
	fs_write(fd, rom, len);
	fs_close(fd);

	cksum = i = 0;
	while(i < len) {
		cksum ^= ((uint32_t *)rom)[i/4];
		i += 4;
	}

	printf("ROM checksum = %08x\n", cksum);

	return rom;

error:
	if (rom)
		free(rom);
	if (fd)
		fs_close(fd);
	return NULL;
}
