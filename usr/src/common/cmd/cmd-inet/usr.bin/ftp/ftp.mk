#      @(#)ftp.mk	1.2 STREAMWare for Unixware 2.0  source
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1994 Lachman Technology, Inc.
# All rights reserved.
#
#      SCCS IDENTIFICATION
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#ident	"@(#)ftp.mk	1.3"
#ident	"$Header$"

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
INSDIR=		$(USRBIN)
OWN=		bin
GRP=		bin

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP

LDLIBS=		-lsocket -lnsl -lresolv

OBJS=		cmds.o cmdtab.o ftp.o getpass.o glob.o main.o pclose.o \
		ruserpass.o domacro.o security.o Signal.o

all:		ftp

ftp:		$(OBJS)
		$(CC) -o ftp  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ftp

clean:
		rm -f $(OBJS) core a.out

clobber:	clean
		rm -f ftp

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

cmds.o:		cmds.c \
		ftp_var.h \
		pathnames.h \
		$(INC)/sys/param.h \
		$(INC)/setjmp.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/arpa/ftp.h \
		$(INC)/arpa/inet.h \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/netdb.h \
		$(INC)/ctype.h \
		$(INC)/sys/wait.h \
		$(FRC)

cmdtab.o:	cmdtab.c \
		ftp_var.h \
		$(INC)/sys/param.h \
		$(INC)/setjmp.h \
		$(FRC)

cmds.o:		cmds.c \
		ftp_var.h \
		pathnames.h \
		$(INC)/sys/param.h \
		$(INC)/setjmp.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/arpa/ftp.h \
		$(INC)/arpa/inet.h \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/netdb.h \
		$(INC)/ctype.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		../../usr.sbin/security.h \
		$(FRC)

cmdtab.o:	cmdtab.c \
		ftp_var.h \
		$(INC)/sys/param.h \
		$(INC)/setjmp.h \
		$(FRC)

domacro.o:	domacro.c \
		ftp_var.h \
		$(INC)/sys/param.h \
		$(INC)/setjmp.h \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(FRC)

ftp.o:		ftp.c \
		ftp_var.h \
		$(INC)/setjmp.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/sys/param.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/ftp.h \
		$(INC)/arpa/telnet.h \
		$(INC)/arpa/inet.h \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/netdb.h \
		$(INC)/fcntl.h \
		$(INC)/pwd.h \
		$(INC)/varargs.h \
		$(INC)/ctype.h \
		$(FRC)

getpass.o:	getpass.c \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/sgtty.h \
		$(FRC)

glob.o:		glob.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/stat.h \
		$(INC)/dirent.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/pwd.h \
		$(FRC)

gtdtablesize.o:		gtdtablesize.c \
		$(INC)/sys/resource.h \
		$(FRC)

main.o:		main.c \
		ftp_var.h \
		$(INC)/sys/param.h \
		$(INC)/setjmp.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/arpa/ftp.h \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/pwd.h \
		$(INC)/varargs.h \
		$(FRC)

pclose.o:	pclose.c \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/sys/param.h \
		$(INC)/sys/wait.h \
		$(FRC)

ruserpass.o:	ruserpass.c \
		$(INC)/stdio.h \
		$(INC)/utmp.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/errno.h \
		$(FRC)

security.o:	../../usr.sbin/security.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../../usr.sbin/security.h \
		$(FRC)
		$(CC) $(CFLAGS) $(DEFLIST) -I../../usr.sbin -c \
			../../usr.sbin/security.c
