# $Id$

TARGET = gen-emu.elf
OBJS = main.o loader.o input.o SN76489.o Sound.o m68k.o z80.o vdp.o misc.o 
OBJS += md5c.o m68k/m68k.o z80/z80.o video.o init.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $(OBJS) $(KOS_LIBS)

m68k/m68k.o:
	@$(MAKE) -C m68k

z80/z80.o:
	@$(MAKE) -C z80

clean:
	rm -f gen-emu.elf $(OBJS)
	@$(MAKE) -C m68k clean
	@$(MAKE) -C z80 clean

include Makefile.kos
