#ident	"@(#)compat.mk	2.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE= compat.mk
KBASE   = ../../..
LINTDIR = $(KBASE)/lintdir
DIR     = io/odi/compat

all:

install:all

clean:

clobber:clean

compatHeaders = \
	dlpi_lsl.h \
	uwodi.h \
	ethtsm.h \
	sr_route.h

headinstall:$(compatHeaders)
	@for f in $(compatHeaders);     \
	do                              \
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)    \
			-g $(GRP) $$f;  \
	done

include $(UTSDEPEND)
