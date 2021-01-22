#note: no cart in this one

# Paths to TMS9900 compilation tools
GAS=/cygdrive/c/cygwin/home/tursi/bin/tms9900-as
LD=/cygdrive/c/cygwin/home/tursi/bin/tms9900-ld
CC=/cygdrive/c/cygwin/home/tursi/bin/tms9900-gcc
CP=/usr/bin/cp
NAME=gromcfg

# Path to elf2cart conversion utility
ELF2CART=/cygdrive/c/cygwin/home/tursi/elf2cart
ELF2EA5=/cygdrive/c/cygwin/home/tursi/elf2ea5
EA5PLIT=/cygdrive/c/cygwin/home/tursi/ea5split/ea5split

# Flags used during linking
#
# Locate the code (.text section) and the data (.data section)
LDFLAGS_EA5=\
  --section-start .text=a000 --section-start .data=2080 -M

INCPATH=-I../libti99
LIBPATH=-L../libti99
LIBS=-lti99

C_FLAGS=\
  -O2 -std=c99 -s --save-temp

# List of compiled objects used in executable
OBJECT_LIST_EA5=\
  crt0_ea5.o\

OBJECT_LIST=\
  main.o

# List of all files needed in executable
PREREQUISITES=\
  $(OBJECT_LIST_EA5) $(OBJECT_LIST)
  
all: gromcfg gromloadbuild
	/cygdrive/c/work/classic99paste/release/classic99paste -reset xx15DSK0.GROMCFG\\n

# Recipe to compile the executable
gromcfg: $(PREREQUISITES)
	$(LD) $(OBJECT_LIST_EA5) $(OBJECT_LIST) $(LIBS) $(LIBPATH) $(LDFLAGS_EA5) -o $(NAME).ea5.elf > ea5.map
	$(ELF2EA5) $(NAME).ea5.elf $(NAME).ea5.bin
	$(EA5PLIT) $(NAME).ea5.bin
	$(CP) GROMCF* /cygdrive/c/classic99/dsk1/

gromloadbuild: $(OBJECT_LIST_EA5) main.c
	$(CC) -D LOADONLY -c main.c $(C_FLAGS) $(INCPATH) -o mainload.o
	$(LD) $(OBJECT_LIST_EA5) mainload.o $(LIBS) $(LIBPATH) $(LDFLAGS_EA5) -o gromload.ea5.elf > ea5b.map
	$(ELF2EA5) gromload.ea5.elf gromload.ea5.bin
	$(EA5PLIT) gromload.ea5.bin
	$(CP) GROMLOA* /cygdrive/c/classic99/dsk1/

gromload: gromloadbuild
	/cygdrive/c/work/classic99paste/release/classic99paste -reset xx15DSK0.GROMLOAD\\n

split:
	$(EA5PLIT) $(NAME).ea5.bin
	$(ELF2CART) $(NAME).c.elf $(NAME).c.bin
	$(CP) GROMCF* /cygdrive/c/classic99/dsk1/

# Recipe to clean all compiled objects
.phony clean:
	rm *.o
	rm *.elf
	rm *.map
	rm *.bin

# Recipe to compile all assembly files

%.o: %.asm
	$(GAS) $< -o $@

# Recipe to compile all C files
%.o: %.c
	$(CC) -c $< $(C_FLAGS) $(INCPATH) -o $@
