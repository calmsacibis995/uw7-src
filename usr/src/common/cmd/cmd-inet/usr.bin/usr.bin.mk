#ident	"@(#)usr.bin.mk	1.2"
#ident	"$Header$"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

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

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP
INSDIR=		$(USRBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen

#	include 'rcp', 'rlogin' and 'rsh' in PRODUCTS list
#	because they don't have to be installed with set[gu]id on anymore
PRODUCTS=	rcp rlogin rsh finger rdate ruptime rwho whois ttcp
DIRS=		ftp netstat ntalk rdist talk tftp telnet

.o:
		$(CC) -o $@ $(CFLAGS) $(DEFLIST)$(LDFLAGS) $< $(LDLIBS) \
			$(SHLIBS)


all:		$(PRODUCTS)
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk all";\
			$(MAKE) -f $$i.mk all $(MAKEARGS);\
			cd ..;\
		done;\
		wait

install:	all
		@for i in $(PRODUCTS);\
		do\
			$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $$i;\
		done
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk install";\
			$(MAKE) -f $$i.mk install $(MAKEARGS);\
			cd ..;\
		done;\
		wait

rcp:		rcp.o security.o
		$(CC) -o $@ $(LDFLAGS) rcp.o security.o $(LDLIBS) $(SHLIBS)

rlogin:		rlogin.o security.o
		$(CC) -o $@ $(LDFLAGS) rlogin.o security.o $(LDLIBS) $(SHLIBS)

rsh:		rsh.o security.o
		$(CC) -o $@ $(LDFLAGS) rsh.o security.o $(LDLIBS) $(SHLIBS)

finger:		finger.o
		$(CC) -o $@ $(LDFLAGS) finger.o $(LDLIBS) $(SHLIBS)

rdate:		rdate.o
		$(CC) -o $@ $(LDFLAGS) rdate.o $(LDLIBS) $(SHLIBS)

ruptime:	ruptime.o
		$(CC) -o $@ $(LDFLAGS) ruptime.o $(LDLIBS) $(SHLIBS)

rwho:		rwho.o
		$(CC) -o $@ $(LDFLAGS) rwho.o $(LDLIBS) $(SHLIBS)

whois:		whois.o
		$(CC) -o $@ $(LDFLAGS) whois.o $(LDLIBS) $(SHLIBS)

ttcp:		ttcp.o
		$(CC) -o $@ $(LDFLAGS) ttcp.o $(LDLIBS) $(SHLIBS)


clean:
		rm -f *.o core a.out
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk clean";\
			$(MAKE) -f $$i.mk clean $(MAKEARGS);\
			cd ..;\
		done;\
		wait
clobber: clean
		rm -f *.o $(PRODUCTS) core a.out
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk clobber";\
			$(MAKE) -f $$i.mk clobber $(MAKEARGS);\
			cd ..;\
		done;\
		wait
lintit:
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk lintit";\
			$(MAKE) -f $$i.mk lintit $(MAKEARGS);\
			cd ..;\
		done;\
		wait

FRC:

#
# Header dependencies
#

finger.o:	finger.c \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/utmp.h \
		$(INC)/sys/signal.h \
		$(INC)/pwd.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(FRC)

rcp.o:		rcp.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/time.h \
		$(INC)/sys/ioctl.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/pwd.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/errno.h \
		$(INC)/dirent.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

rdate.o:	rdate.c \
		$(INC)/signal.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(FRC)

rlogin.o:	rlogin.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/file.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/stropts.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/sgtty.h \
		$(INC)/pwd.h \
		$(INC)/signal.h \
		$(INC)/setjmp.h \
		$(INC)/netdb.h \
		$(INC)/fcntl.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/sockio.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

rsh.o:		rsh.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/file.h \
		$(INC)/sys/filio.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/pwd.h \
		$(INC)/netdb.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

ruptime.o:	ruptime.c \
		$(INC)/sys/param.h \
		$(INC)/stdio.h \
		$(INC)/dirent.h \
		$(INC)/protocols/rwhod.h \
		$(FRC)

rwho.o:		rwho.c \
		$(INC)/sys/param.h \
		$(INC)/stdio.h \
		$(INC)/dirent.h \
		$(INC)/protocols/rwhod.h \
		$(FRC)

whois.o:	whois.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		$(FRC)

security.o:	../usr.sbin/security.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)
		$(CC) $(CFLAGS) $(DEFLIST) -I../usr.sbin -c ../usr.sbin/security.c
