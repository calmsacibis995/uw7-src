#ident	"@(#)ip.mk	1.4"

include $(CMDRULES)

LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../ppp/include

LOCALDEF= -DSYSV -DSVR4 -Kthread -DSTATIC=

INSDIR=		$(USRSBIN)
LIBDIR=		$(USRLIB)/ppp/psm
OWN=		bin
GRP=		bin
LDLIBS=		-lresolv -lsocket -lnsl -lgen -lthread -lx -laas -lfilter
LDFLAGS=	-L $(USRLIB)

RT_CFILES = ip_rt.c ip_log.c arp.c
RT_FILES = ip_rt.o ip_log.o arp.o
RT = ip_rt.so

PARSE_CFILES = ip_parse.c
PARSE_FILES = ip_parse.o
PARSE = ip_parse.so

SRCFILES = $(RT_CFILES) $(PARSE_CFILES)
FILES = $(RT_FILES) $(PARSE_FILES)
TARGS = $(RT) $(PARSE)

all:		$(TARGS)

clean:
	-rm -f $(FILES)

clobber:	clean
	-rm -f $(TARGS)

config:

headinstall:

install:	all
	[ -d $(LIBDIR) ] || mkdir -p $(LIBDIR)
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) $(RT)
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) $(PARSE)
	$(INS) -f $(LIBDIR) -m 0755 -u $(OWN) -g $(GRP) ipexec.sh
	$(INS) -f $(LIBDIR) -m 0755 -u $(OWN) -g $(GRP) ipfilters

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(RT):		$(RT_FILES)
		$(CC) -KPIC -G -o $(RT) $(RT_FILES) $(LDLIBS)

$(PARSE):	$(PARSE_FILES)
		$(CC) -KPIC -G -o $(PARSE) $(PARSE_FILES)
