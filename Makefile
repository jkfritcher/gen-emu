# $Id$

TARGET = gen-emu.elf
OBJS = main.o loader.o memory.o misc.o md5c.o m68k/m68k.o mz80/mz80.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $(OBJS) $(KOS_LIBS)

m68k/m68k.o:
	@$(MAKE) -C m68k

mz80/mz80.o:
	@$(MAKE) -C mz80

clean:
	rm -f gen-emu.elf $(OBJS)
	@$(MAKE) -C m68k clean
	@$(MAKE) -C mz80 clean

include Makefile.kos
