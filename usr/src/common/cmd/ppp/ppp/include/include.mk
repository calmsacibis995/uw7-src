#ident	"@(#)include.mk	1.2"

#
# This makefile builds pppd ppptalk the ppp shell and all required
# libraries (libpppparse.so libppprt.so)
#

include $(CMDRULES)

CFLAGS = -g 
LDLIBS = -lsocket -lgen -lthread -lx -L .. -lnsl
LOCALDEF = -DSYSV -DSVR4 -Kthread -DSTATIC=
OWN=		bin
GRP=		bin
LIBDIR =	$(USRLIB)/ppp
LIBDIRBASE =	/usr/lib/ppp
ETCPPPD =	$(ETC)/ppp.d

#
# Header files for /usr/include
#
INC_HEADER_FILES = \
	ppptalk.h \
	psm.h

PPP_HEADER_FILES = \
	act.h \
	malloc.h \
	ppp_cfg.h \
	ppp_proto.h \
	fsm.h \
	pathnames.h \
	ppp_type.h \
	ulr.h \
	lcp_hdr.h \
	paths.h

all:	

clean:

clobber: clean

config:

headinstall:

install:	all

fnames:

lintit:

#
# Header dependencies
#
