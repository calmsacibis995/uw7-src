#ident "@(#)lib.idi.mk	29.1"
#
#   Name : lib.idi.gemini.M
#
#   COPYRIGHT NOTICE:
#
#   Copyright (C) Eicon Technology Corporation, 1993.
#   This module contains Proprietary Information of
#   Eicon Technology Corporation, and should be treated
#   as confidential.
#
#   Description :
#   Gemini Makefile for lib.idi component

include $(UTSRULES)

# Directories of interest:

MAKEFILE = lib.idi.mk
SCP_SRC = diload.c
KBASE = ../../../../..
# DEBUG = -DDEBUG
LOCALDEF = -DUNIX -DUNIXWARE -DNEW_DRIVER
LOGEXE = dilog
LOGOBJ = dilog.o libdimaint.a
LIBDIMAINTOBJ = log.o maintlib.o revise.o
LOGLOBJS = dilog.L 
LOADEXE = ../EtdD.cf/divaload 
LOADOBJ = diload.o libdiload.a 
LIBDILOADOBJ = load.o
LOADLOBJS = diload.L 

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(LOADEXE) $(MAKEARGS); \
	fi

diload.o: diload.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c diload.c

libdimaint.a: log.o maintlib.o revise.o

log.o: log.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c log.c
	$(AR) rv libdimaint.a $@

maintlib.o: maintlib.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c maintlib.c
	$(AR) rv libdimaint.a $@

revise.o: revise.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c revise.c
	$(AR) rv libdimaint.a $@

dilog.o: dilog.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c dilog.c

libdiload.a: load.o 

load.o: load.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c load.c
	$(AR) rv libdiload.a $@
	
$(LOADEXE): diload.o libdiload.a
	$(CC) -o $@ $(LOADOBJ)   

$(LOGEXE): dilog.o libdimaint.a
	$(CC) -o $@ $(LOGOBJ)   

lintit: $(LOADLOBJS) $(LOGLOBJS)

install:

clean:
	rm -f $(LOADOBJ) $(LIBDILOADOBJ)

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(LOADEXE); \
	fi


