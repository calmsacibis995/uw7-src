#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/rdist/rdist.mk	1.1"
#ident	"$Header$"

#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.

#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 		PROPRIETARY NOTICE (Combined)
# 
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
# 
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
# 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#       (c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  
#

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP -D_KMEMUSER -DSVR4 
INSDIR=		$(USRBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen

OBJS= 		docmd.o expand.o lookup.o main.o server.o gram.o security.o

CLEANFILES=y.tab.h

all:		rdist

rdist:		$(OBJS)
		$(CC) -o rdist  $(INCLIST) $(DEFLIST) $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f ${INSDIR} -m 4555 -u $(OWN) -g $(GRP) rdist

clean:
		rm -f $(OBJS) $(CLEANFILES) core a.out

clobber:	clean
		rm -f rdist

lintit:
		$(LINT) $(LINTFLAGS) *.c


FRC:

#
# Header dependencies
#


docmd.o:	docmd.c \
		defs.h \
		pathnames.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fs/s5param.h \
		$(INC)/sys/types.h \
		$(INC)/dirent.h \
		$(INC)/sys/dirent.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/sys/fcntl.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/limits.h \
		$(INC)/setjmp.h \
		$(INC)/netdb.h \
		$(FRC)

expand.o:	expand.c \
		defs.h \
		pathnames.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fs/s5param.h \
		$(INC)/sys/types.h \
		$(INC)/dirent.h \
		$(INC)/sys/dirent.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/sys/fcntl.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/limits.h \
		$(FRC)

lookup.o:	lookup.c \
		defs.h \
		pathnames.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fs/s5param.h \
		$(INC)/sys/types.h \
		$(INC)/dirent.h \
		$(INC)/sys/dirent.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/sys/fcntl.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/limits.h \
		$(FRC)

main.o:		main.c \
		defs.h \
		pathnames.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fs/s5param.h \
		$(INC)/sys/types.h \
		$(INC)/dirent.h \
		$(INC)/sys/dirent.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/sys/fcntl.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/limits.h \
		$(FRC)

security.o:	../../usr.sbin/security.c \
		../../usr.sbin/security.h
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c ../../usr.sbin/security.c

server.o:	server.c \
		defs.h \
		pathnames.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fs/s5param.h \
		$(INC)/sys/types.h \
		$(INC)/dirent.h \
		$(INC)/sys/dirent.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/sys/fcntl.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/limits.h \
		$(FRC)

