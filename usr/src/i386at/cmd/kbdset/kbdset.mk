#ident	"@(#)kbdset:kbdset.mk	1.2.1.2"
#ident "$Header: "

include	$(CMDRULES)

all:	kbdset

kbdset:	kbdset.o
	$(CC) kbdset.o -o $@ $(LDFLAGS)

install:	all
	$(INS) -f $(USRBIN) kbdset

clean:
	rm -f *.o

clobber:	clean
	rm -f kbdset
