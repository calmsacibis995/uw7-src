#	copyright	"%c%"

#ident	"@(#)uniq:uniq.mk	1.2.6.2"

include $(CMDRULES)

#	Makefile for uniq

OWN = bin
GRP = bin

LDLIBS = -lw

all: uniq

uniq: uniq.c
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uniq

clean:
	rm -f uniq.o

clobber: clean
	rm -f uniq

lintit:
	$(LINT) $(LINTFLAGS) uniq.c
