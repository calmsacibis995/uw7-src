#ident	"@(#)pppsh.mk	1.3"
#
# This makefile builds the ppp shell
#

include $(CMDRULES)

LDLIBS = -lsocket -lnsl -lgen -lx  -L.. -lpppcmd
LOCALDEF = -DSYSV -DSVR4 -DSTATIC=
LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../include

OWN=		bin
GRP=		bin
LIBDIR =	$(USRLIB)/ppp
LIBDIRBASE =	/usr/lib/ppp

PPPSH_CFILES = pppsh.c
PPPSH_FILES = pppsh.o
PPPSH = pppsh

SRCFILES = $(PPPSH_CFILES)
FILES = $(PPPSH_FILES)
TARGS = $(PPPSH)

all:	$(TARGS)

clean:
	-rm -f $(FILES)

clobber: clean
	-rm -f $(TARGS)

config:

headinstall:

install:	all
	[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(PPPSH)

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(PPPSH):	$(PPPSH_FILES)
		$(CC) -o $(PPPSH) $(LDFLAGS) $(PPPSH_FILES) \
			$(LDLIBS) $(SHLIBS)

#
# Header dependencies
#
