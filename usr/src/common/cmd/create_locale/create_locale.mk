#	copyright	"1"

#ident	"@(#)create_locale.mk	1.2"
#	Makefile for create_locale

include $(CMDRULES)

INSDIR = $(USRBIN)

MAINS = create_locale

SOURCES =  create_loc.sh

all:	$(MAINS)

create_locale:	 $(SOURCES)
	cp create_loc.sh create_locale

clean:
	rm -f $(MAINS)

clobber: clean

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 $(MAINS)

