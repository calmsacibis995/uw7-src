#		copyright	"%c%"
#ident	"@(#)Makefile	1.3"

#
# Makefile for the hpnp binary commands
#

include $(CMDRULES)


TOP	=	../..

include ../../common.mk


LOCALINC= -I. -I$(LPINC)
LDLIBS = -lnsl -lsocket -lgen


HDRS	= \
		snmp.h

SRCS	= \
		canaccess.c \
		chkaccess.c \
		fifo.c \
		gethostaddr.c \
		hpnpf.c \
		hpnptyd.c \
		log.c \
		network.c \
		status.c

OBJS	=	$(SRCS:.c=.o)

LINTLB  =	../lib/llib-llplib.ln

CMDS	= \
		hpnpf \
		hpnptyd \
		gethostaddr \
		canaccess


all:		$(CMDS)


install:	all
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP) hpnpf; \
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP) hpnptyd; \
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP)/cfg gethostaddr; \
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP)/cfg canaccess;


clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMDS)

strip:
	$(STRIP) $(CMDS)


hpnpf:		hpnpf.o chkaccess.o network.o fifo.o log.o status.o \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o hpnpf hpnpf.o chkaccess.o network.o fifo.o log.o status.o \
			$(LDFLAGS) \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

hpnptyd:	hpnptyd.o network.o fifo.o log.o \
			$(LIBREQ) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o hpnptyd hpnptyd.o network.o fifo.o log.o $(LDFLAGS) \
			$(LIBREQ) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

gethostaddr:	gethostaddr.o \
			$(LIBPRT) \
			$(LIBREQ) \
			$(LIBBSD) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o gethostaddr gethostaddr.o $(LDFLAGS) \
			$(LIBPRT) \
			$(LIBREQ) \
			$(LIBBSD) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

canaccess:	canaccess.o chkaccess.o network.o log.o \
			$(LIBFLT) \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o canaccess canaccess.o chkaccess.o network.o log.o $(LDFLAGS) \
			$(LIBFLT) \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)


lintit:
	$(LINT) $(DEFLIST) comb.c \
			$(LINTLB) 
	$(LINT) $(DEFLIST) cancel.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lp.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpfilter.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpforms.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpmove.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpshut.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpusers.c \
			$(LINTLB)

lintlib:
