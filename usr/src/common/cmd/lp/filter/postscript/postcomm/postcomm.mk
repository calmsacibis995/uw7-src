#	copyright	"%c%"

#ident	"@(#)postcomm.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

#
# makefile for the program that sends files to PostScript printers.
#

MAKEFILE=postcomm.mk
ARGS=all

#
# Common source and header files have been moved to $(COMMONDIR).
#

COMMONDIR=../common

#
# postcomm doesn't use floating point arithmetic, so the -f flag isn't needed.
#

LOCALDEF= -DSYSV
LOCALINC= -I$(COMMONDIR)

#CFILES=postcomm.c ifdef.c
CFILES=postcomm.c

HFILES=postcomm.h

POSTIO=$(CFILES:.c=.o)

ALLFILES=README $(MAKEFILE) $(HFILES) $(CFILES)


all : postcomm

install : postcomm
	@if [ ! -d "$(BINDIR)" ]; then \
	    mkdir $(BINDIR); \
	    $(CH)chmod 775 $(BINDIR); \
	    $(CH)chgrp $(GROUP) $(BINDIR); \
	    $(CH)chown $(OWNER) $(BINDIR); \
	fi
	$(INS) -m 775 -u $(OWNER) -g $(GROUP) -f $(BINDIR) postcomm
#	cp postcomm $(BINDIR)
#	chmod 775 $(BINDIR)/postcomm
#	chgrp $(GROUP) $(BINDIR)/postcomm
#	chown $(OWNER) $(BINDIR)/postcomm

postcomm : $(POSTIO)
	if [ -d "$(DKHOSTDIR)" ]; \
	    then $(CC) -o postcomm $(POSTIO) $(LDFLAGS) -Wl,-L$(DKHOSTDIR)/lib -ldk $(SHLIBS); \
	    else $(CC) -o postcomm $(POSTIO) $(LDFLAGS) $(SHLIBS); \
	fi

postcomm.o : $(HFILES)

clean :
	rm -f *.o

clobber : clean
	rm -f postcomm

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:

