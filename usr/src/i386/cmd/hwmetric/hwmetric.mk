#		copyright	"%c%"
#ident	"@(#)hwmetric:hwmetric.mk	1.4"
#ident	"$Header$"

# Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved.

include	$(CMDRULES)

OWN	=	bin
GRP	=	bin

CFLAGS  = -DHEADERS_INSTALLED
OBJS = main.o do.o pfmt.o pml.o rc.o cpu.o cpumtr.o cg.o cgmtr.o

INSDIR = $(USRSBIN)
INSETC = $(ETC)

all:	hwmetric 

install:	all
	-rm -f $(INSDIR)/hwmetric
	-rm -f $(INSLIB)/hwmetricrc
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) hwmetric
	$(INS) -f $(INSETC) -m 0444 -u $(OWN) -g $(GRP) hwmetricrc

hwmetric: $(OBJS)
	$(CC) $(LDFLAGS) -o hwmetric $(OBJS)

main.o: main.c hwmetric.h
	$(CC) $(CFLAGS) -c main.c

do.o: do.c hwmetric.h
	$(CC) $(CFLAGS) -c do.c

pfmt.o: pfmt.c hwmetric.h
	$(CC) $(CFLAGS) -c pfmt.c

pml.o: pml.c hwmetric.h
	$(CC) $(CFLAGS) -c pml.c

rc.o: rc.c hwmetric.h
	$(CC) $(CFLAGS) -c rc.c

cpu.o: cpu.c hwmetric.h
	$(CC) $(CFLAGS) -c cpu.c

cpumtr.o: cpumtr.c hwmetric.h
	$(CC) $(CFLAGS) -c cpumtr.c

cg.o: cg.c hwmetric.h
	$(CC) $(CFLAGS) -c cg.c

cgmtr.o: cgmtr.c hwmetric.h
	$(CC) $(CFLAGS) -c cgmtr.c

clean:
	-rm -f $(OBJS)

clobber:	clean
	-rm -f hwmetric
