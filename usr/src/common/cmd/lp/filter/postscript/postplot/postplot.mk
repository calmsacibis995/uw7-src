#	copyright	"%c%"

#ident	"@(#)postplot.mk	1.2"
#ident "$Header$"
#
# postplot makefile
#

include $(CMDRULES)

MAKEFILE=postplot.mk
ARGS=all

#
# Common header and source files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

LOCALDEF= -DSYSV
LOCALINC= -I$(COMMONDIR)

CFILES=postplot.c \
       $(COMMONDIR)/request.c \
       $(COMMONDIR)/glob.c \
       $(COMMONDIR)/misc.c

HFILES=postplot.h \
       $(COMMONDIR)/comments.h \
       $(COMMONDIR)/ext.h \
       $(COMMONDIR)/gen.h \
       $(COMMONDIR)/path.h

POSTPLOT=postplot.o\
       $(COMMONDIR)/request.o\
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : postplot

install : postplot
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) postplot
#	cp postplot $(BINDIR)
#	$(CH)chmod 775 $(BINDIR)/postplot
#	$(CH)chgrp $(GROUP) $(BINDIR)/postplot
#	$(CH)chown $(OWNER) $(BINDIR)/postplot

postplot : $(POSTPLOT)
	$(CC) -o postplot $(POSTPLOT) $(LDFLAGS) $(SHLIBS)

$(COMMONDIR)/glob.o : $(COMMONDIR)/glob.c $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c glob.c

$(COMMONDIR)/misc.o : $(COMMONDIR)/misc.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c misc.c

$(COMMONDIR)/request.o : $(COMMONDIR)/request.c $(COMMONDIR)/ext.h $(COMMONDIR)/gen.h $(COMMONDIR)/path.h
	cd $(COMMONDIR); $(CC) $(CFLAGS) $(DEFLIST) -c request.c

postplot.o : $(HFILES)

clean :
	rm -f $(POSTPLOT)

clobber : clean
	rm -f postplot

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:
