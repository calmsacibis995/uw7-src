#ident "@(#)tools.mk	5.2"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1993-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)

PROG=cvbinCdecl
OBJ=cvbinCdecl.o

.c.o:
	$(HCC) -c -O $<

all: $(PROG)

$(PROG): $(OBJ)
	$(HCC) -O -o $(PROG) $(OBJ)

install: all

clean:
	rm -f $(OBJ)

lintit:
	
clobber: clean
	rm -f $(PROG)
