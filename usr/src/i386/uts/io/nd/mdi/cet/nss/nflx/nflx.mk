
# Compiler stuff. Note that this will also compile under "rcc".

include $(UTSRULES)

DFLAGS=-D_KERNEL
INCLUDE=../include
DEPENDS=$(INCLUDE)/CPQTYPES.H $(INCLUDE)/NSS.H $(INCLUDE)/NFLX.H \
	$(INCLUDE)/NFLXPROT.H
CFLAGS=-O -I$(INCLUDE) -I. $(DFLAGS)
#CC=cc

OUTPUT_FILES=../../NetFlex.a

# Primary rules

install: all

all:	$(OUTPUT_FILES)

touch:
	touch -c *.C *.H

clobber: clean

clean:
	rm -f $(OUTPUT_FILES) *.o

# Secondary rules

../../NetFlex.a: NFLXRX.o NFLXTX.o NFLXEVNT.o NFLXINIT.o NFLXREQ.o
	ld -r -o ../../NetFlex.a NFLXRX.o NFLXTX.o NFLXEVNT.o NFLXINIT.o \
		 NFLXREQ.o

NFLXRX.o:	NFLXRX.c $(DEPENDS)
		$(CC) -c $(CFLAGS) NFLXRX.c

NFLXTX.o:	NFLXTX.c $(DEPENDS)
		$(CC) -c $(CFLAGS) NFLXTX.c

NFLXEVNT.o:	NFLXEVNT.c $(DEPENDS)
		$(CC) -c $(CFLAGS) NFLXEVNT.c

NFLXINIT.o:	NFLXINIT.c $(DEPENDS)
		$(CC) -c $(CFLAGS) NFLXINIT.c

NFLXREQ.o:	NFLXREQ.c $(DEPENDS)
		$(CC) -c $(CFLAGS) NFLXREQ.c

