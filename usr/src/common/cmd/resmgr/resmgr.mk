#ident	"@(#)resmgr:resmgr.mk	1.2"

include	$(CMDRULES)

MAINS= resmgr resmgr.dy
LDLIBS= -lgen -lresmgr

all:	$(MAINS)

install:	all 
	$(INS) -f $(SBIN) resmgr
	$(INS) -f $(SBIN) resmgr.dy

clean:
	rm -f *.o

clobber: clean
	rm -f $(MAINS)

resmgr:	resmgr.o
	$(CC) -o resmgr resmgr.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

resmgr.dy:	resmgr.o
	$(CC) -o resmgr.dy resmgr.o $(LDFLAGS) $(LDLIBS)
