#ident	"@(#)lprof:bblk/i386/bblk.mk	1.7"
#
# makefile for basicblk
#

include $(CMDRULES)

SGSBASE	= ../../..
INS	= $(SGSBASE)/sgs.install

DBG	= #-gvDDEBUG
OBJS	= bblk.o dwarf.o mach.o parse.o util.o
INCS	= -I../$(CPU) -I../common -I$(CPUINC) -I$(COMINC)

BBLK	= ../../basicblk

all: $(BBLK)

bblk.o: ../common/bblk.c ../common/bblk.h
	$(CC) -c $(CFLAGS) $(DBG) $(INCS) ../common/bblk.c

dwarf.o: ../common/dwarf.c ../common/bblk.h
	$(CC) -c $(CFLAGS) $(DBG) $(INCS) ../common/dwarf.c

mach.o: ../$(CPU)/mach.c ../common/bblk.h
	$(CC) -c $(CFLAGS) $(DBG) $(INCS) ../$(CPU)/mach.c

parse.o: ../common/parse.c ../common/bblk.h
	$(CC) -c $(CFLAGS) $(DBG) $(INCS) ../common/parse.c

util.o: ../common/util.c ../common/bblk.h
	$(CC) -c $(CFLAGS) $(DBG) $(INCS) ../common/util.c

$(BBLK): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

install: all
	cp $(BBLK) basicblk.bak
	$(STRIP) $(BBLK)
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/$(SGS)basicblk $(BBLK)
	mv basicblk.bak $(BBLK)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(BBLK)
