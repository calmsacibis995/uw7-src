#ident "@(#)tcpcfg.mk	1.3"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1993 Lachman Technology, Inc.
# All rights reserved.
#
include $(CMDRULES)

INSDIR=$(ROOT)/$(MACH)/usr/lib/netcfg/bin
LDLIBS=-lsocket
LOCAL_DEFS=-DGET_DEFAULT

all: tcpcfg

tcpcfg: tcpcfg.c 
	$(CC) $(CFLAGS) $(LOCAL_DEFS) $(LDFLAGS) -o tcpcfg tcpcfg.c $(LDLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) tcpcfg

clean:
	-rm -f tcpcfg.o

clobber: clean
	-rm -f tcpcfg
