#ident	"@(#)ppptalk.mk	1.3"

#
# This makefile builds pppd and required libraries (libppprt.so)
#

include $(CMDRULES)

LDLIBS = -lsocket -lgen  -lx -L .. -lnsl -lpppcmd -ledit
LOCALDEF = -DSYSV -DSVR4 -DSTATIC=
LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../include

OWN=		bin
GRP=		bin
LIBDIR =	$(USRLIB)/ppp
LIBDIRBASE =	/usr/lib/ppp

PPPTALK_MSG = ppptalk_msg.h
PPPTALK_GEN = NLS/english/ppptalk.gen

PPPTALK_CFILES = ppptalk.c cmd.c defs.c
PPPTALK_FILES = ppptalk.o cmd.o defs.o
PPPTALK = ppptalk

PARSE_CFILES = parse.c
PARSE_FILES = parse.o
PARSE = libpppparse.so

SRCFILES = $(PPPTALK_CFILES) $(PARSE_CFILES)
FILES = $(PPPTALK_FILES) $(PARSE_FILES)
TARGS = $(PARSE) $(PPPTALK)


all:	$(TARGS)

clean:
	-rm -f $(FILES) $(PPPTALK_MSG)
	$(DOCATS) -d NLS $@

clobber: clean
	-rm -f $(TARGS)
	$(DOCATS) -d NLS $@

config:

headinstall:

install:	all
	[ -d $(LIBDIR) ] || mkdir -p $(LIBDIR)
	$(INS) -f $(LIBDIR) -m 0555 -u $(OWN) -g $(GRP) $(PARSE)
	[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(PPPTALK)
	$(DOCATS) -d NLS $@

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(PPPTALK_MSG):	$(PPPTALK_GEN)
		$(MKCATDEFS) ppptalk $(PPPTALK_GEN) > /dev/null

$(PPPTALK):	$(PPPTALK_MSG) $(PPPTALK_FILES)
		$(CC) -o $(PPPTALK) $(LDFLAGS) $(PPPTALK_FILES) \
			$(LDLIBS) $(SHLIBS) -L. -lpppparse

$(PARSE):	$(PARSE_FILES)
		$(CC) -KPIC -G -h $(LIBDIRBASE)/$(PARSE) \
			-o $(PARSE) $(PARSE_FILES)

#
# Header dependencies
#
