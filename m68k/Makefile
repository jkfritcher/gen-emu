
CC = gcc
CFLAGS = $(KOS_CFLAGS) -Iobj -I.

OBJS = obj/m68kcpu.o obj/m68kopac.o obj/m68kopdm.o obj/m68kopnz.o obj/m68kops.o

all: m68k.a

m68k.a: obj $(OBJS)
	$(KOS_AR) rcs $@ $(OBJS)

obj:
	mkdir obj

obj/m68kcpu.o: m68kcpu.c obj/m68kops.h
	$(KOS_CC) $(CFLAGS) -c -o $@ $<

obj/m68kopac.o: obj/m68kopac.c obj/m68kops.h
	$(KOS_CC) $(CFLAGS) -c -o $@ $<

obj/m68kopdm.o: obj/m68kopdm.c obj/m68kops.h
	$(KOS_CC) $(CFLAGS) -c -o $@ $<

obj/m68kopnz.o: obj/m68kopnz.c obj/m68kops.h
	$(KOS_CC) $(CFLAGS) -c -o $@ $<

obj/m68kops.o: obj/m68kops.c obj/m68kops.h
	$(KOS_CC) $(CFLAGS) -c -o $@ $<

obj/m68kops.h: m68kmake
	m68kmake obj m68k_in.c

m68kmake: m68kmake.c m68k_in.c
	$(CC) -g -O0 -Wall -o $@ $<

clean:
	/bin/rm -Rf obj m68kmake m68k.a

include ../Makefile.kos
