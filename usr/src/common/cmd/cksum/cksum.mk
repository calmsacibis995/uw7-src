#	copyright	"%c%"

#ident	"@(#)cksum:cksum.mk	1.1.2.2"

include $(CMDRULES)

#	Makefile for cksum

OWN = bin
GRP = bin

LOCALDEF = -D_FILE_OFFSET_BITS=64

all: cksum

cksum: cksum.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFLIST) -o cksum cksum.o cksum.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) cksum

clean:
	rm -f cksum.o

clobber: clean
	rm -f cksum

lintit:
	$(LINT) $(LINTFLAGS) cksum.c

#	These targets are useful but optional

partslist:
	@echo cksum.mk cksum.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo cksum | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit cksum.mk cksum.c -o cksum.o cksum
