#ident	"@(#)kbdload:kbdload.mk	1.1.1.2"
#ident "$Header: "

include $(CMDRULES)

OFILES	= loader.o main.o scrutiny.o util.o
MAIN	= kbdload


all:	$(MAIN)

kbdload: $(OFILES)
	$(CC) $(OFILES) -o $(MAIN) $(LDFLAGS)

install:	all
	$(INS) -f $(USRBIN) $(MAIN)

clean:
	rm -f *.o

clobber:	clean
	rm -f $(MAIN)
