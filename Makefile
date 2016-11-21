# $Id$

TARGET = gen-emu.elf
OBJS = main.o loader.o input.o SN76489.o Sound.o m68k.o z80.o vdp.o
OBJS += md5c.o m68k/m68k.o z80/z80.o init.o
CFLAGS = -g3 -O0 -Wall -I/usr/local/include
LDFLAGS = -L/usr/local/lib
LIBS = -lSDL2

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

m68k/m68k.o:
	@$(MAKE) -C m68k

z80/z80.o:
	@$(MAKE) -C z80

clean:
	rm -f gen-emu.elf $(OBJS)
	@$(MAKE) -C m68k clean
	@$(MAKE) -C z80 clean
