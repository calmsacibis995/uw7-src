#ident	"@(#)ws.mk	1.13"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	ws.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/ws

WS = ws.cf/Driver.o
LFILE = $(LINTDIR)/ws.ln

FILES = \
	ws_tables.o \
	ws_cmap.o \
	ws_ansi.o \
	ws_tcl.o \
	ws_subr.o \
	ws_8042.o

CFILES = \
	ws_tables.c \
	ws_cmap.c \
	ws_ansi.c \
	ws_subr.c \
	ws_tcl.c \
	ws_8042.c

LFILES = \
	ws_tables.ln \
	ws_cmap.ln \
	ws_ansi.ln \
	ws_subr.ln \
	ws_tcl.ln \
	ws_8042.ln

all: $(WS)

install: all
	(cd ws.cf; $(IDINSTALL) -R$(CONF) -M ws)

$(WS): $(FILES)
	$(LD) -r -o $(WS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(WS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ws 

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done

#
# Header Install Section
#

sysHeaders = \
	vt.h
syswsHeaders = \
	8042.h \
	chan.h \
	tcl.h \
	ws.h

headinstall: $(sysHeaders) $(syswsHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@-[ -d $(INC)/sys/ws ] || mkdir -p $(INC)/sys/ws
	@for f in $(syswsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/ws -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
