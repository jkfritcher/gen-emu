# $Id$

CFLAGS = $(KOS_CFLAGS) $(KOS_ALL_INCS) -DLSB_FIRST
#CFLAGS = $(KOS_CFLAGS) $(KOS_ALL_INCS)

all: z80.o

z80.o: z80.c
	$(KOS_CC) $(CFLAGS) -c -o $@ $<

clean:
	/bin/rm -f z80.o

include ../Makefile.kos
