#ident	"@(#)libpppcmd.mk	1.3"

#
# This makefile builds libpppcmd
#

include $(CMDRULES)

LDLIBS = -lsocket -lgen -lx -lnsl
LOCALDEF = -DSYSV -DSVR4 -DSTATIC=
LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../include

LIBDIR =	$(USRLIB)/ppp
LIBDIRBASE =	/usr/lib/ppp

PPPCMD_CFILES = sock_suppt.c
PPPCMD_FILES =  sock_supt.o
PPPCMD = ../libpppcmd.a

SRCFILES = $(PPPCMD_CFILES)
FILES = $(PPPCMD_FILES)
TARGS = $(PPPCMD)

all:	$(TARGS)

clean:
	-rm -f $(FILES)

clobber: clean
	-rm -f $(TARGS)

config:

headinstall:

install:	all

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(PPPCMD):	$(PPPCMD_FILES)
		$(AR) -ru $(PPPCMD) $(PPPCMD_FILES)

#
# Header dependencies
#
