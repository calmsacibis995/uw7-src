#ident	"@(#)ntalk.mk	1.2"
#ident	"$Header$"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
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

#LOCALDEF=	-DSYSV 
INSDIR=		$(USRBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lcurses -lsocket -lnsl

OBJS=		talk.o get_names.o display.o io.o ctl.o init_disp.o\
	  	msgs.o get_addrs.o ctl_trans.o invite.o look_up.o

all:		talk

talk:		$(OBJS)
		$(CC) -o talk  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) talk

clean:
		rm -f $(OBJS) core a.out

clobber:	clean
		rm -f talk

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

ctl.o:		ctl.c \
		talk_ctl.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

ctl_trans.o:	ctl_trans.c \
		talk_ctl.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(INC)/sys/time.h \
		$(FRC)

display.o:	display.c \
		talk.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

get_addrs.o:	get_addrs.c \
		talk_ctl.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

get_names.o:	get_names.c \
		talk.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/param.h \
		$(INC)/pwd.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

init_disp.o:	init_disp.c \
		talk.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(INC)/signal.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/stropts.h \
		$(FRC)

invite.o:	invite.c \
		talk_ctl.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(INC)/sys/time.h \
		$(INC)/signal.h \
		$(INC)/setjmp.h \
		$(FRC)

io.o:		io.c \
		talk.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/sys/time.h \
		$(FRC)

look_up.o:	look_up.c \
		talk_ctl.h \
		$(INC)/protocols/talkd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

msgs.o:		msgs.c \
		talk.h \
		$(INC)/protocols/talkd.h \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(INC)/sys/time.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

talk.o:		talk.c \
		talk.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)


