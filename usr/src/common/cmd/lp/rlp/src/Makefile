#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#
# Makefile for the rlpstat and rlpcmd commands
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I. -I$(LPINC)
LDLIBS = -lgen

#LOCALDEF= -DDEBUG

HDRS	= 

SRCS	= \
		rlpstat.c \
		rlpcmd.c

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

RLPSTAT =	rlpstat
RLPCMD =	rlpcmd

CMDS	= \
		$(RLPSTAT) \
		$(RLPCMD)


all:		$(CMDS)

install:	all
	$(INS) -m $(SMODES) -u $(SUPER) -g $(GROUP) -f $(USRBIN) $(RLPSTAT)
	$(INS) -m $(SMODES) -u $(SUPER) -g $(GROUP) -f $(USRBIN) $(RLPCMD)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMDS)

strip:
	$(STRIP) $(CMDS)

$(RLPSTAT):		$(OBJS) $(LIBS)
	$(CC) -o $(RLPSTAT) $(RLPSTAT).o $(LDFLAGS) $(LIBS) $(SYSLIBS) $(SHLIBS) $(LDLIBS)

$(RLPCMD):		$(OBJS) $(LIBS)
	$(CC) -o $(RLPCMD) $(RLPCMD).o $(LDFLAGS) $(LIBS) $(SYSLIBS) $(SHLIBS) $(LDLIBS)

$(OBJS):	$(HDRS)

lintit:
	$(LINT) $(DEFLIST) $(SRCS) $(LINTLB)

lintlib:
	$(LINT) -abhmuvxy -o lpstt $(DEFLIST) $(SRCS)

