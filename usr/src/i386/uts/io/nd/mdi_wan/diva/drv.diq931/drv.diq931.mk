#ident "@(#)drv.diq931.mk	29.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

include $(UTSRULES)

MAKEFILE = drv.diq931.mk
SCP_SRC = pc.c
KBASE = ../../../../..
# DEBUG = -DDEBUG
LOCALDEF =  -DUNIX -DUNIXWARE -D_KERNEL -D_INKERNEL -DNEW_DRIVER -DI_HLC \
			-DI_MSN -DI_OLS $(DEBUG)
OBJ = util.o cause.o xlat.o task.o output.o dec.o trans.o
NI1OBJS = $(OBJ) pc.ni1.o
5ESSOBJS = $(OBJ) pc.5ess.o
ETSIOBJS = $(OBJ) pc.etsi.o
JAPANOBJS = $(OBJ) pc.japan.o
ATELOBJS = $(OBJ) pc.atel.o
LOBJS = util.L cause.L xlat.L task.L output.L dec.L trans.L pc.L
DIPROT = ../drv.diprot/Driver.o
DRV = Driver.etsi.o Driver.ni1.o Driver.5ess.o Driver.japan.o Driver.atel.o

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi

Driver.etsi.o: $(DIPROT) $(ETSIOBJS)
	$(LD) -r -o $@ $(DIPROT) $(ETSIOBJS)

Driver.ni1.o: $(DIPROT) $(NI1OBJS)
	$(LD) -r -o $@ $(DIPROT) $(NI1OBJS)

Driver.5ess.o: $(DIPROT) $(5ESSOBJS)
	$(LD) -r -o $@ $(DIPROT) $(5ESSOBJS)

Driver.atel.o: $(DIPROT) $(ATELOBJS)
	$(LD) -r -o $@ $(DIPROT) $(ATELOBJS)

Driver.japan.o: $(DIPROT) $(JAPANOBJS)
	$(LD) -r -o $@ $(DIPROT) $(JAPANOBJS)

$(DIPROT):
	(cd ../drv.diprot; make -f drv.diprot.mk $(MAKEARGS))

pc.etsi.o:	pc.c
	$(CC) -o $@ $(CFLAGS) $(INCLIST) $(DEFLIST) -DETSI -c pc.c

pc.ni1.o:	pc.c
	$(CC) -o $@ $(CFLAGS) $(INCLIST) $(DEFLIST) -DNIX -c pc.c

pc.5ess.o:	pc.c
	$(CC) -o $@ $(CFLAGS) $(INCLIST) $(DEFLIST) -DATT -c pc.c

pc.atel.o:	pc.c
	$(CC) -o $@ $(CFLAGS) $(INCLIST) $(DEFLIST) -DAUSTRAL -c pc.c

pc.japan.o:	pc.c
	$(CC) -o $@ $(CFLAGS) $(INCLIST) $(DEFLIST) -DJAPAN -c pc.c

install:

lintit:	$(LOBJS)

clean: 
	rm -f $(DRV) $(ETSIOBJS) $(NI1OBJS) $(5ESSOBJS) $(ATELOBJS) $(JAPANOBJS) $(LOBJS) tags

clobber: clean
