#	copyright	"%c%"

#ident	"@(#)postmd.mk	1.2"
#ident "$Header$"

#
# makefile for the matrix display program.
#

include $(CMDRULES)

MAKEFILE=postmd.mk
ARGS=all

#
# Common header and source files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

LOCALDEF= -DSYSV
LOCALINC= -I$(COMMONDIR)
LDLIBS = -lm

CFILES=postmd.c \
       $(COMMONDIR)/request.c \
       $(COMMONDIR)/glob.c \
       $(COMMONDIR)/misc.c

HFILES=postmd.h \
       $(COMMONDIR)/comments.h \
       $(COMMONDIR)/ext.h \
       $(COMMONDIR)/gen.h \
       $(COMMONDIR)/path.h

POSTMD=postmd.o\
       $(COMMONDIR)/request.o \
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : postmd

install : postmd
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) postmd
#	cp postmd $(BINDIR)
#	chmod 775 $(BINDIR)/postmd
#	chgrp $(GROUP) $(BINDIR)/postmd
#	chown $(OWNER) $(BINDIR)/postmd

postmd : $(POSTMD)
	$(CC) -o postmd $(POSTMD) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(COMMONDIR)/glob.o : $(COMMONDIR)/glob.c $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c glob.c

$(COMMONDIR)/misc.o : $(COMMONDIR)/misc.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c misc.c

$(COMMONDIR)/request.o : $(COMMONDIR)/request.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h $(COMMONDIR)/path.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c request.c

postmd.o : $(HFILES)

clean :
	rm -f $(POSTMD)

clobber : clean
	rm -f postmd

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:
