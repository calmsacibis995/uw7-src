#ident	"@(#)chap.mk	1.3"

include $(CMDRULES)

LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../ppp/include

LOCALDEF= -DSVR4 -DSYSV -Kthread -DSTATIC=
INSDIR=		$(USRSBIN)
LIBDIR=		$(USRLIB)/ppp/psm
OWN=		bin
GRP=		bin
LDLIBS=		-lsocket -lnsl -lgen -lthread -lx
LDFLAGS=	-L $(USRLIB)

RT_CFILES = chap_rt.c
RT_FILES = chap_rt.o
RT = chap_rt.so

PARSE_CFILES = chap_parse.c
PARSE_FILES = chap_parse.o
PARSE = chap_parse.so

SRCFILES= $(RT_CFILES) $(PARSE_CFILES)
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

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(RT):		$(RT_FILES)
		$(CC) -KPIC -G -o $(RT) $(RT_FILES) -L ../md5 -lmd5

$(PARSE):	$(PARSE_FILES)
		$(CC) -KPIC -G -o $(PARSE) $(PARSE_FILES)
