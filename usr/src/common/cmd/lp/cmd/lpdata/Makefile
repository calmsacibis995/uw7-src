#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for the lpdata command
#


include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I. -I$(LPINC)
#LOCALDEF=-DDEBUG

SRCS	= \
		lpdata.c

OBJS	=	$(SRCS:.c=.o)

LIBS	= \
		$(LIBMSG) \
		$(LIBFRM) \
		$(LIBREQ) \
		$(LIBPRT) \
		$(LIBCLS) \
		$(LIBACC) \
		$(LIBFLT) \
		$(LIBUSR) \
		$(LIBLP)  \
		$(LIBOAM)

LINTLB  =	../llib-llpcmd.ln $(TOP)/lib/llib-llplib.ln

SYSLIBS	=	-lgen

LINTS	= \
		$(LINTMSG) \
		$(LINTFRM) \
		$(LINTREQ) \
		$(LINTPRT) \
		$(LINTCLS) \
		$(LINTACC) \
		$(LINTFLT) \
		$(LINTUSR) \
		$(LINTLP)  \
		$(LINTOAM)


CMD	=	lpdata


all:		$(CMD)

install:
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBLP) $(CMD)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMD)

strip:
	$(STRIP) $(CMD)

$(CMD):		$(OBJS) $(LIBS)
	$(CC) -o $(CMD) $(OBJS) $(LDFLAGS) $(LIBS) $(SYSLIBS) $(SHLIBS)

lintit:
	$(LINT) $(DEFLIST) $(SRCS) $(LINTLB)
 
lintlib:
	$(LINT) -abhmuvxy -o lpdat $(DEFLIST) $(SRCS)
