#ident	"@(#)pcnfsd.mk	1.4"
#ident	"$Header$"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

OWN = bin
GRP = bin

BINS= pcnfsd
OBJS= pcnfsd_svc.o pcnfsd_xdr.o pcnfsd_v1.o pcnfsd_v2.o pcnfsd_misc.o \
	pcnfsd_cache.o pcnfsd_print.o

SRCS= $(OBJS:.o=.c)
INCSYS = $(INC)
INSDIR = $(USRLIB)/nfs

LINTFLAGS= -hbax $(DEFLIST)
COFFLIBS= -lrpc -ldes -lsocket -lnsl_s -lgen -liaf -lcrypt
#ELFLIBS = -lrpcsvc -dy -lnsl -lrpc -ldes -lnet -lsocket -lgen
ELFLIBS = -L /usr/ucblib -dy -lsocket -lnsl -lgen -liaf -lcrypt
LDLIBS = `if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(SHLIBS) $(LDLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags -tw $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
