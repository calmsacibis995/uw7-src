#	copyright	"%c%"

#ident	"@(#)postreverse.mk	1.2"
#ident "$Header$"

#
# makefile for the page reversal utility program.
#

include $(CMDRULES)

MAKEFILE=postreverse.mk
ARGS=all

#
# Common header and source files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

#
# postreverse doesn't use floating point arithmetic, so the -f flag isn't needed.
#

LOCALINC= -I$(COMMONDIR)

CFILES=postreverse.c \
       $(COMMONDIR)/glob.c \
       $(COMMONDIR)/misc.c \
       $(COMMONDIR)/tempnam.c

HFILES=postreverse.h \
       $(COMMONDIR)/ext.h \
       $(COMMONDIR)/comments.h \
       $(COMMONDIR)/gen.h \
       $(COMMONDIR)/path.h

POSTREVERSE=postreverse.o\
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o\
       $(COMMONDIR)/tempnam.o

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : postreverse

install : postreverse
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) postreverse
#	cp postreverse $(BINDIR)
#	chmod 775 $(BINDIR)/postreverse
#	chgrp $(GROUP) $(BINDIR)/postreverse
#	chown $(OWNER) $(BINDIR)/postreverse

postreverse : $(POSTREVERSE)
	$(CC) -o postreverse $(POSTREVERSE) $(LDFLAGS) $(SHLIBS)

$(COMMONDIR)/glob.o : $(COMMONDIR)/glob.c $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c glob.c

$(COMMONDIR)/misc.o : $(COMMONDIR)/misc.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c misc.c

$(COMMONDIR)/tempnam.o : $(COMMONDIR)/tempnam.c
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c tempnam.c

postreverse.o : $(HFILES)


clean :
	rm -f $(POSTREVERSE)

clobber : clean
	rm -f postreverse

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:
