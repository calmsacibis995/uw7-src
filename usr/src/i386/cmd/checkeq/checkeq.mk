#	copyright	"%c%"


#ident	"@(#)checkeq:i386/cmd/checkeq/checkeq.mk	1.1.4.2"
#ident	"$Header$"

# Makefile for checkeq

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

all: checkeq

checkeq:	checkeq.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

checkeq.o:

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) checkeq

clean:
	rm -f checkeq.o

clobber: clean
	rm -f checkeq
