#ident	"@(#)pppd.mk	1.3"

#
# This makefile builds pppd ppptalk the ppp shell and all required
# libraries (libpppparse.so libppprt.so)
#

include $(CMDRULES)

LDLIBS = -lsocket -lgen -lthread -lx -L .. -lnsl
LOCALDEF = -DSYSV -DSVR4 -Kthread -DSTATIC=
LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../include

OWN=		bin
GRP=		bin
LIBDIR =	$(USRLIB)/ppp
LIBDIRBASE =	/usr/lib/ppp
ETCPPPD =	$(ETC)/ppp.d

PPPD_CFILES = pppd.c
PPPD_FILES = pppd.o
PPPD = pppd

RT_CFILES = start.c  timer.c ulr.c fsm.c link.c util.c cd.c psm.c fsm_cfg.c\
	ucfg.c act.c bundle.c phase.c auth.c  init.c hist.c
RT_FILES = start.o  timer.o ulr.o fsm.o link.o util.o cd.o psm.o fsm_cfg.o\
	ucfg.o act.o bundle.o phase.o auth.o  init.o hist.o
RT = libppprt.so

SRCFILES =  $(PPPD_CFILES) $(RT_CFILES) 
FILES =  $(PPPD_FILES) $(RT_FILES) 
TARGS = $(RT)  $(PPPD) 

.MUTEX: $(TARGS)

all:	$(TARGS)

clean:
	-rm -f $(FILES)

clobber: clean
	-rm -f $(TARGS)

config:

headinstall:

install:	all
	[ -d $(LIBDIR) ] || mkdir -p $(LIBDIR)
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) $(RT)
	[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) $(PPPD)
	[ -d $(ETCPPPD) ] || mkdir -p $(ETCPPPD)
	$(INS) -f $(ETCPPPD) -m 0644 -u $(OWN) -g $(GRP) .pppcfg

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(PPPD):	$(PPPD_FILES)
		$(CC) -o $(PPPD) $(LDFLAGS) $(PPPD_FILES)\
			$(LDLIBS) $(SHLIBS) -L. -lppprt

$(RT):		$(RT_FILES)
		$(CC) -KPIC -G -h $(LIBDIRBASE)/$(RT) \
			 -o$(RT) $(RT_FILES) -lx		

#
# Header dependencies
#
