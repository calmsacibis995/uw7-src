#	copyright	"%c%"

#ident	"@(#)pg:pg.mk	1.9.1.4"
#ident "$Header$"

include $(CMDRULES)


OWN = bin
GRP = bin

LDLIBS = -lgen -lw -lcurses

all: pg

pg: pg.o
	$(CC) -o pg pg.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pg

clean:
	rm -f pg.o

clobber: clean
	rm -f pg

lintit:
	$(LINT) $(LINTFLAGS) *.c
