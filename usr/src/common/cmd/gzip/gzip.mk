#	Copyright (c) 1993 UNIVEL

#ident	"@(#)gzip.mk	1.2"
#ident "$Header$"

include $(CMDRULES)


OWN = bin
GRP = bin

all: gzip

gzip:
	make -f Makefile

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) gzip

clean:
	-make -f Makefile clean

clobber: clean
	make -f Makefile clean

lintit:
	$(LINT) $(LINTFLAGS) *.c
