#ident	"@(#)mouse.mk	1.4"

include	$(CMDRULES)

all:	mousemgr mouseadmin

install:	all 
	$(INS) -f $(USRBIN) mouseadmin
	$(INS) -f $(USRLIB) -m 0775 -u root -g sys mousemgr
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 mousemgr.str

clean:
	rm -f *.o

clobber: clean
	rm -f mousemgr mouseadmin

mousemgr:	mousemgr.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS)

mouseadmin:	mouseadmin.o
	$(CC) -o mouseadmin mouseadmin.o $(LDFLAGS) $(LDLIBS) -lcurses -lresmgr
