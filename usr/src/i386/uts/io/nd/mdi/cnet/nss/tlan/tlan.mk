
include $(UTSRULES)

# Compiler stuff. Note that this will also compile under "rcc".

DFLAGS=-D_INKERNEL
INCLUDE=../include
DEPENDS=$(INCLUDE)/CPQTYPES.H $(INCLUDE)/NSS.H $(INCLUDE)/TLAN.H \
	$(INCLUDE)/TLANPROT.H
CFLAGS=-O -I$(INCLUDE) -I. $(DFLAGS) -DUSE_TLAN_30_FEATURES -DCAYENNE -DLEVEL_ONE -DLIBRARY_HANDLES_TLAN23_TX_FIFO_BUG
# CC=cc

OUTPUT_FILES=../../Tlan.a

# Primary rules

install: all

all:	$(OUTPUT_FILES)

touch:
	touch -c *.C *.H

clobber: clean

clean:
	rm -f $(OUTPUT_FILES) *.o

# Secondary rules

../../Tlan.a: TLANRX.o TLANTX.o TLANEVNT.o TLANINIT.o TLANREQ.o TLANHW.o \
		 TLANDIAG.o
	ld -r -o ../../Tlan.a TLANRX.o TLANTX.o TLANEVNT.o TLANINIT.o \
		 TLANREQ.o TLANHW.o TLANDIAG.o

TLANRX.o:	TLANRX.c $(DEPENDS)
		$(CC) -c $(CFLAGS) TLANRX.c

TLANTX.o:	TLANTX.c $(DEPENDS)
		$(CC) -c $(CFLAGS) TLANTX.c

TLANEVNT.o:	TLANEVNT.c $(DEPENDS)
		$(CC) -c $(CFLAGS) TLANEVNT.c

TLANINIT.o:	TLANINIT.c $(DEPENDS)
		$(CC) -c $(CFLAGS) TLANINIT.c

TLANREQ.o:	TLANREQ.c $(DEPENDS)
		$(CC) -c $(CFLAGS) TLANREQ.c

TLANDIAG.o:	TLANDIAG.c $(DEPENDS)
		$(CC) -c $(CFLAGS) TLANDIAG.c

TLANHW.o:	TLANHW.c $(DEPENDS)
		$(CC) -c $(CFLAGS) TLANHW.c

