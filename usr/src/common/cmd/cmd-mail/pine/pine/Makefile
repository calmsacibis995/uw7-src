#ident	"@(#)Makefile	11.1"

SHELL	= /bin/sh

LOCALDEF=	-I. -Dconst= -DSV4 -DSYSTYPE=\"SV4\" -DMOUSE
LIBS	=	../c-client/libclient.a ../pico/libpico.a -ltermlib -lsocket -lgen

include $(CMDRULES)

LDLIBS	=	-L$(USRLIB)

build:		pine

OBJS =		addrbook.o \
		adrbklib.o \
		args.o \
		context.o \
		date.o \
		filter.o \
		folder.o \
		help.o \
		helptext.o \
		imap.o \
		init.o \
		mailcap.o \
		mailcmd.o \
		mailindx.o \
		mailpart.o \
		mailview.o \
		newmail.o \
		os.o \
		other.o \
		pine.o \
		reply.o \
		screen.o \
		send.o \
		signals.o \
		status.o \
		strings.o \
		ttyin.o \
		ttyout.o

pine:		$(OBJS)
		$(CC) $(LDFLAGS) -o pine $(OBJS) $(LDLIBS) $(LIBS)

$(OBJS):	helptext.h

helptext.c:	helptext.h
		sh cmplhelp.sh < pine.hlp > helptext.c

helptext.h:	pine.hlp
		sh cmplhlp2.sh < pine.hlp > helptext.h

includer:	includer.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o includer includer.c $(LDLIBS)

clean:
		rm -f $(OBJS) helptext.h helptext.c

clobber:	clean
		rm -f pine
