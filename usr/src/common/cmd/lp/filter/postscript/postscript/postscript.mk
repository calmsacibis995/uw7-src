#	copyright	"%c%"

#ident	"@(#)postscript.mk	1.2"
#ident "$Header$"

#
# A makefile for installing PostScript library files.
#

include $(CMDRULES)

MAKEFILE=postscript.mk
ARGS=compile

#
# If $(WANTROUND) is true file $(ROUNDPAGE) is appended to the installed prologue
# files listed in $(ROUNDPROGS).
#

WANTROUND=false
ROUNDPAGE=roundpage.ps
ROUNDPROGS=dpost postprint postdaisy

LIBFILES=*.ps ps.*
ALLFILES=README $(MAKEFILE) $(LIBFILES)


all :

install :
	@if [ ! -d "$(LIBDIR)" ]; then \
	    mkdir $(LIBDIR); \
	    $(CH)chmod 775 $(LIBDIR); \
	    $(CH)chgrp $(GROUP) $(LIBDIR); \
	    $(CH)chown $(OWNER) $(LIBDIR); \
	fi
	for f in $(LIBFILES); \
	do \
		$(INS) -m 664 -u $(OWNER) -g $(GROUP) -f $(LIBDIR) $$f; \
	done
#	cp $(LIBFILES) $(LIBDIR)
#	@if [ "$(WANTROUND)" = "true" ]; then \
#	    for i in $(ROUNDPROGS); do \
#		if [ -w $(LIBDIR)/$$i.ps ]; then \
#		    cat $(ROUNDPAGE) >>$(LIBDIR)/$$i.ps; \
#		fi; \
#	    done; \
#	fi
#	cd $(LIBDIR); \
#	chmod 664 $(LIBFILES); \
#	chgrp $(GROUP) $(LIBFILES); \
#	chown $(OWNER) $(LIBFILES)

clean clobber :

list :
	pr -n $(ALLFILES) | $(LIST)

lintit:
