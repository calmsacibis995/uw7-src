#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for the lpstat command
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I. -I$(LPINC)
LDLIBS = -lgen

#LOCALDEF= -DDEBUG

HDRS	= \
		lpstat.h

SRCS	= \
		accept.c \
		add_mounted.c \
		charset.c \
		class.c \
		device.c \
		done.c \
		form.c \
		lpstat.c \
		output.c \
		parse.c \
		printer.c \
		request.c \
		send_message.c

OBJS	=	$(SRCS:.c=.o)

LINTLB  =	../llib-llpcmd.ln ../../lib/llib-llplib.ln

LIBS	= \
		$(LIBACC) \
		$(LIBCLS) \
		$(LIBMSG) \
		$(LIBPRT) \
		$(LIBFRM) \
		$(LIBLP)  \
		$(LIBOAM)

SYSLIBS =	-lcurses

CMD	=	lpstat


all:		$(CMD)

install:	all
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRBIN) $(CMD)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMD)

strip:
	$(STRIP) $(CMD)

$(CMD):		$(OBJS) $(LIBS)
	$(CC) -o $(CMD) $(OBJS) $(LDFLAGS) $(LIBS) $(SYSLIBS) $(SHLIBS) $(LDLIBS)

$(OBJS):	$(HDRS)

lintit:
	$(LINT) $(DEFLIST) $(SRCS) $(LINTLB)

lintlib:
	$(LINT) -abhmuvxy -o lpstt $(DEFLIST) $(SRCS)

