
CC = gcc
XCC = sh-elf-gcc
XLD = sh-elf-ld
#CFLAGS = -g -O0 -Wall -Iobj -I.
CFLAGS = -O2 -Wall -Iobj -I.

OBJS = obj/m68kopnz.o obj/m68kopdm.o obj/m68kopac.o obj/m68kops.o obj/m68kcpu.o

all: m68k.o

m68k.o: obj $(OBJS)
	$(XLD) -r --oformat elf32-shl -o m68k.o $(OBJS)

obj:
	mkdir obj

obj/m68kcpu.o: obj/m68kops.h m68kcpu.c
	$(XCC) -ml -m4-single-only $(CFLAGS) -c -o obj/m68kcpu.o m68kcpu.c

obj/m68kops.o: obj/m68kops.h obj/m68kops.c
	$(XCC) -ml -m4-single-only $(CFLAGS) -c -o obj/m68kops.o obj/m68kops.c

obj/m68kopac.o: obj/m68kops.h obj/m68kopac.c
	$(XCC) -ml -m4-single-only $(CFLAGS) -c -o obj/m68kopac.o obj/m68kopac.c

obj/m68kopdm.o: obj/m68kops.h obj/m68kopdm.c
	$(XCC) -ml -m4-single-only $(CFLAGS) -c -o obj/m68kopdm.o obj/m68kopdm.c

obj/m68kopnz.o: obj/m68kops.h obj/m68kopnz.c
	$(XCC) -ml -m4-single-only $(CFLAGS) -c -o obj/m68kopnz.o obj/m68kopnz.c

obj/m68kops.h: m68kmake
	m68kmake obj m68k_in.c

m68kmake: m68kmake.c m68k_in.c
	$(CC) -g -O0 -Wall -o m68kmake m68kmake.c

clean:
	/bin/rm -Rf obj m68kmake m68k.o
