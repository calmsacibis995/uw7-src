#ident	"@(#)md5.mk	1.3"

include $(CMDRULES)

LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../ppp/include

LOCALDEF= -DSYSV -DSVR4 -Kthread -DSTATIC=
INSDIR=		$(USRSBIN)
LIBDIR=		$(USRLIB)
OWN=		bin
GRP=		bin
LDLIBS=		-lsocket -lnsl -lgen -lthread -lx
LDFLAGS=	-L $(USRLIB)

MD5_CFILES = md5c.c
MD5_FILES = md5c.o
MD5 = libmd5.so

SRCFILES = $(MD5_CFILES)
FILES = $(MD5_FILES)
TARGS = $(MD5)

all:		$(TARGS)

clean:
	-rm -f $(FILES)

clobber:	clean
	-rm -f $(TARGS)

config:

headinstall:

install:	all
	[ -d $(LIBDIR) ] || mkdir -p $(LIBDIR)
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) $(TARGS)

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(MD5):		$(MD5_FILES)
		$(CC) -KPIC -G -o $(MD5) $(MD5_FILES)

