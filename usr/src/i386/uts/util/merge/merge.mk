#	copyright	"%c%"

#ident	"@(#)kern-i386:util/merge/merge.mk	1.3.2.2"
#ident	"$Header$"

# This makefile is just installing the header
include $(UTSRULES)

MAKEFILE=	merge.mk
KBASE = ../..
DIR = util/merge

MERGE = merge.cf/Driver.o

all:

install:	all

clean:

clobber:	clean

lintit:

fnames:

sysHeaders = \
	merge386.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC: 

include $(UTSDEPEND)

include $(MAKEFILE).dep
