#ident	"@(#)kern-i386at:io/ramd/ramd.mk	1.8"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	ramd.mk
KBASE    = ../..
CNFFILE  = ramd.cf/Driver.o

CFILES = ramd.c
FILES	 = ramd.o

SRCFILES = $(CFILES)

CFILES = $(FILES:.o=.c)

all :	$(CNFFILE) 

install: all
	(cd ramd.cf; $(IDINSTALL) -R$(CONF) -M ramd  )

$(CNFFILE):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#

#
# Cleanup Section
#
clean:
	-rm -f $(FILES) $(CNFFILE)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d ramd


lint:

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

headinstall: \
	$(KBASE)/io/ramd/ramd.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/io/ramd/ramd.h
	

FRC:


include $(UTSDEPEND)

include $(MAKEFILE).dep
