#ident	"@(#)kbdpipe:kbdpipe.mk	1.2.1.3"
#ident "$Header: "

include $(CMDRULES)

all: kbdpipe

kbdpipe: kbdpipe.o
	$(CC) kbdpipe.o -o $@ $(LDFLAGS)

install:	all
	$(INS) -f $(USRBIN) kbdpipe

clean:
	rm -f *.o

clobber:	clean
	rm -f kbdpipe
