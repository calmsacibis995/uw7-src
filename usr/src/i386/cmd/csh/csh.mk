#	copyright	"%c%"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#ident	"@(#)csh:i386/cmd/csh/csh.mk	1.7.12.1"
#ident "$Header$"

#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley Software License Agreement
# specifies the terms and conditions for redistribution.
#
#
# C Shell with process control; VM/UNIX VAX Makefile
# Bill Joy UC Berkeley; Jim Kulp IIASA, Austria
#
# To profile, put -DPROF in DEFS and -pg in CFLAGS, and recompile.

include $(CMDRULES)

.KEEP_STATE:
.FRC:

INSDIR = $(USRBIN)
OWN = bin
GRP = bin
DFLGS = -DVPIX
BINS = csh
LOCAL_HDRS = sh.h sh.local.h sh.dir.h param.h sh.char.h sh.proc.h sh.tconst.h sh.msg.h

# -DVFORK has been turned off for the time being as a workaround for some
# monstrous memory leaks.  Performance suffers considerably as a result,
# so it is hoped that we can revert to using vfork eventually.
# LOCALDEF = -DRCHECK -DBSD_COMP -DTELL -DVFORK -DFILEC -Um32 $(MBCHAR) $(DFLGS)

LOCALDEF = -DRCHECK -DBSD_COMP -DTELL -DFILEC -Um32 $(MBCHAR) $(DFLGS)
LOCALINC = -I.
LDLIBS= -lcurses -lw -lgen

SRCS=	printf.c sh.c sh.char.c sh.debug.c sh.dir.c sh.dol.c sh.err.c \
	sh.exec.c sh.exp.c sh.file.c sh.func.c sh.glob.c sh.hist.c sh.init.c \
	sh.lex.c sh.misc.c sh.parse.c sh.print.c sh.proc.c sh.sem.c sh.set.c \
	sh.tchar.c sh.tconst.c sh.time.c stubs.c setpgrp.c \
	bcopy.c bzero.c rindex.c getwd.c gethostname.c getrusage.c killpg.c \
	wait3.c signal.c index.c alloc.c setpriority.c getpagesize.c sh.msg.c

OBJS=	printf.o sh.char.o sh.debug.o sh.dir.o sh.dol.o sh.err.o \
	sh.exec.o sh.exp.o sh.file.o sh.func.o sh.glob.o sh.hist.o sh.init.o \
	sh.lex.o sh.misc.o sh.parse.o sh.print.o sh.proc.o sh.sem.o sh.set.o \
	sh.tchar.o sh.tconst.o sh.time.o sh.o stubs.o setpgrp.o \
	bcopy.o bzero.o rindex.o getwd.o gethostname.o getrusage.o killpg.o \
	wait3.o signal.o index.o alloc.o setpriority.o getpagesize.o sh.msg.o

.INIT:  $(HDRS) $(LOCAL_HDRS)

all: $(BINS)

csh: $(OBJS)
	rm -f csh
	$(CC) $(OBJS) -o $@ $(LDLIBS) $(LDFLAGS) -Kosrcrt $(SHLIBS)
	
install: $(BINS)
	$(INS) -o -m 555 -u $(OWN) -g $(GRP) -f $(INSDIR) csh

install_h:

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f csh
